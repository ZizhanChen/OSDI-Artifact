/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Surface"

#include <stdio.h>

#include "jni.h"
#include <nativehelper/JNIHelp.h>
#include "android_os_Parcel.h"
#include "android/graphics/GraphicBuffer.h"
#include "android/graphics/GraphicsJNI.h"

#include "core_jni_helpers.h"
#include <android_runtime/android_view_Surface.h>
#include <android_runtime/android_graphics_SurfaceTexture.h>
#include <android_runtime/Log.h>

#include <binder/Parcel.h>

#include <gui/Surface.h>
#include <gui/view/Surface.h>
#include <gui/SurfaceControl.h>
#include <gui/GLConsumer.h>

#include <ui/Rect.h>
#include <ui/Region.h>

#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkImage.h>
#include <SkRegion.h>

#include <utils/misc.h>
#include <utils/Log.h>

#include <nativehelper/ScopedUtfChars.h>

#include <AnimationContext.h>
#include <FrameInfo.h>
#include <RenderNode.h>
#include <renderthread/RenderProxy.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "utils/kms.h"
#include <drm_fourcc.h>
#include "zizhan_buffer.h"
#include "libdrm_macros.h"

// ----------------------------------------------------------------------------

namespace android {

static const char* const OutOfResourcesException =
    "android/view/Surface$OutOfResourcesException";

static struct {
    jclass clazz;
    jfieldID mNativeObject;
    jfieldID mLock;
    jmethodID ctor;
} gSurfaceClassInfo;

static struct {
    jfieldID left;
    jfieldID top;
    jfieldID right;
    jfieldID bottom;
} gRectClassInfo;

class JNamedColorSpace {
public:
    // ColorSpace.Named.SRGB.ordinal() = 0;
    static constexpr jint SRGB = 0;

    // ColorSpace.Named.DISPLAY_P3.ordinal() = 7;
    static constexpr jint DISPLAY_P3 = 7;
};

constexpr ui::Dataspace fromNamedColorSpaceValueToDataspace(const jint colorSpace) {
    switch (colorSpace) {
        case JNamedColorSpace::DISPLAY_P3:
            return ui::Dataspace::DISPLAY_P3;
        default:
            return ui::Dataspace::V0_SRGB;
    }
}

// ----------------------------------------------------------------------------

// this is just a pointer we use to pass to inc/decStrong
static const void *sRefBaseOwner;

bool android_view_Surface_isInstanceOf(JNIEnv* env, jobject obj) {
    return env->IsInstanceOf(obj, gSurfaceClassInfo.clazz);
}

sp<ANativeWindow> android_view_Surface_getNativeWindow(JNIEnv* env, jobject surfaceObj) {
    return android_view_Surface_getSurface(env, surfaceObj);
}

sp<Surface> android_view_Surface_getSurface(JNIEnv* env, jobject surfaceObj) {
    sp<Surface> sur;
    jobject lock = env->GetObjectField(surfaceObj,
            gSurfaceClassInfo.mLock);
    if (env->MonitorEnter(lock) == JNI_OK) {
        sur = reinterpret_cast<Surface *>(
                env->GetLongField(surfaceObj, gSurfaceClassInfo.mNativeObject));
        env->MonitorExit(lock);
    }
    env->DeleteLocalRef(lock);
    return sur;
}

jobject android_view_Surface_createFromSurface(JNIEnv* env, const sp<Surface>& surface) {
    jobject surfaceObj = env->NewObject(gSurfaceClassInfo.clazz,
            gSurfaceClassInfo.ctor, (jlong)surface.get());
    if (surfaceObj == NULL) {
        if (env->ExceptionCheck()) {
            ALOGE("Could not create instance of Surface from IGraphicBufferProducer.");
            LOGE_EX(env);
            env->ExceptionClear();
        }
        return NULL;
    }
    surface->incStrong(&sRefBaseOwner);
    return surfaceObj;
}

jobject android_view_Surface_createFromIGraphicBufferProducer(JNIEnv* env,
        const sp<IGraphicBufferProducer>& bufferProducer) {
    if (bufferProducer == NULL) {
        return NULL;
    }

    sp<Surface> surface(new Surface(bufferProducer, true));
    return android_view_Surface_createFromSurface(env, surface);
}

int android_view_Surface_mapPublicFormatToHalFormat(PublicFormat f) {
    return mapPublicFormatToHalFormat(f);
}

android_dataspace android_view_Surface_mapPublicFormatToHalDataspace(
        PublicFormat f) {
    return mapPublicFormatToHalDataspace(f);
}

PublicFormat android_view_Surface_mapHalFormatDataspaceToPublicFormat(
        int format, android_dataspace dataSpace) {
    return mapHalFormatDataspaceToPublicFormat(format, dataSpace);
}
// ----------------------------------------------------------------------------

static inline bool isSurfaceValid(const sp<Surface>& sur) {
    return Surface::isValid(sur);
}

// ----------------------------------------------------------------------------

static jlong nativeCreateFromSurfaceTexture(JNIEnv* env, jclass clazz,
        jobject surfaceTextureObj) {
    sp<IGraphicBufferProducer> producer(SurfaceTexture_getProducer(env, surfaceTextureObj));
    if (producer == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException",
                "SurfaceTexture has already been released");
        return 0;
    }

    sp<Surface> surface(new Surface(producer, true));
    if (surface == NULL) {
        jniThrowException(env, OutOfResourcesException, NULL);
        return 0;
    }

    surface->incStrong(&sRefBaseOwner);
    return jlong(surface.get());
}

static void nativeRelease(JNIEnv* env, jclass clazz, jlong nativeObject) {
    sp<Surface> sur(reinterpret_cast<Surface *>(nativeObject));
    sur->decStrong(&sRefBaseOwner);
}

static jboolean nativeIsValid(JNIEnv* env, jclass clazz, jlong nativeObject) {
    sp<Surface> sur(reinterpret_cast<Surface *>(nativeObject));
    return isSurfaceValid(sur) ? JNI_TRUE : JNI_FALSE;
}

static jboolean nativeIsConsumerRunningBehind(JNIEnv* env, jclass clazz, jlong nativeObject) {
    sp<Surface> sur(reinterpret_cast<Surface *>(nativeObject));
    if (!isSurfaceValid(sur)) {
        doThrowIAE(env);
        return JNI_FALSE;
    }
    int value = 0;
    ANativeWindow* anw = static_cast<ANativeWindow*>(sur.get());
    anw->query(anw, NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND, &value);
    return value;
}

static inline SkColorType convertPixelFormat(PixelFormat format) {
    /* note: if PIXEL_FORMAT_RGBX_8888 means that all alpha bytes are 0xFF, then
        we can map to kN32_SkColorType, and optionally call
        bitmap.setAlphaType(kOpaque_SkAlphaType) on the resulting SkBitmap
        (as an accelerator)
    */
    switch (format) {
    case PIXEL_FORMAT_RGBX_8888:    return kN32_SkColorType;
    case PIXEL_FORMAT_RGBA_8888:    return kN32_SkColorType;
    case PIXEL_FORMAT_RGBA_FP16:    return kRGBA_F16_SkColorType;
    case PIXEL_FORMAT_RGB_565:      return kRGB_565_SkColorType;
    default:                        return kUnknown_SkColorType;
    }
}

//zizhan's try
/*
static void debug_save_file(void *ptr, int size, char *path)
{
	FILE *debug_file;
	int ret = 0;

	debug_file = fopen(path, "wb");
	if (!debug_file) {
		ALOGE("DEBUG: open debug file %s failed!\n", path);
		return;		
	}

	ret = fwrite(ptr, 1, size, debug_file);
	if (ret != size)
		ALOGE("DEBUG: save debug file %s failed\n", path);
	else
		ALOGE("DEBUG: save debug file as %s\n", path);

	fclose(debug_file);
}
*/
/*
static void fill_frame_rgb24_date(unsigned char *ptr, int width, int height, int linesize)
{
	unsigned char *line_ptr = NULL;
	unsigned char d_r = 0, d_g = 0, d_b = 0;

	if (!ptr || (linesize < width)) {
		ALOGE("ERROR: %s invalid args!\n", __func__);
		return;
	}

	for (int i = 0; i < height; i++) {
		line_ptr = ptr + i * linesize;
		for (int j = 0; j < width; j++) {
			if (j < (width / 4)) {
				d_r = 0; d_g = 0; d_b = 0;
			} else if (j < (width / 2)) {
				d_r = 255; d_g = 255; d_b = 255;
			} else if (j < (width * 3 / 4)) {
				d_r = 0; d_g = 0; d_b = 0;
			} else {
				d_r = 255; d_g = 255; d_b = 255;
			}

			*line_ptr = d_r;
			line_ptr++;
			*line_ptr = d_g;
			line_ptr++;
			*line_ptr = d_b;
			line_ptr++;
		}
	}	
}
*/


static int drm_display_zizhan(void * ptr)
{

    int frame_cnt = 30;
    int width, height;
	int ret = 0;
    int drm_fd = 0;
    int connector_id, crtc_id, plane_id;
	
    
    drmModeRes *res = NULL;
    drmModeConnector *conn = NULL;
	//drmModeCrtc *crtc = NULL;
	drmModePlaneRes *pres = NULL;
    drmModePlane *plane = NULL;
	unsigned int fb_id;

	width = 1920;
	height = 1080;
	ALOGE("### Default Resolution: 1920x1080\n");

	ALOGE("[zizhan] ### Init drm ...\n");
    drm_fd = util_open(NULL, NULL);
	if (drm_fd < 0) {
		ALOGE("ERROR: drm_open failed!\n");
		return -1;
	}

	//Get screen infos
	if (drmSetClientCap (drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1)) {
		ALOGE("ERROR: drmModeGetCrtc failed!\n");
		//goto OUT;
        return -1;
	}

	res = drmModeGetResources (drm_fd);
	if (!res) {
		ALOGE("ERROR: drmModeGetResources failed!\n");
		drmClose(drm_fd);
		return -1;
	}

	ALOGE("[zizhan] INFO: count_fbs:%d, count_crtcs:%d, count_encoders:%d, count_connectors:%d \n",
		res->count_fbs, res->count_crtcs, res->count_encoders, res->count_connectors);

	// This test for only 1 connector and only 1 crtc
	connector_id = res->connectors[1];
	crtc_id = res->crtcs[1];

	// Just fort print connector only
	conn = drmModeGetConnector (drm_fd, connector_id);
	if (!conn) {
		ALOGE("ERROR: drmModeGetConnector failed!\n");
		//goto OUT;
        return -1;
	}

	// Get plane infos
	pres = drmModeGetPlaneResources (drm_fd);
	if (!pres) {
		ALOGE("ERROR: drmModeGetPlaneResources failed!\n");
		//goto OUT;
        return -1;
	}
    ALOGE("[zizhan]INFO: count_planes: %d", pres->count_planes);
    for (int i=0; i<pres->count_planes; i++) {
        plane_id = pres->planes[i];
        plane = drmModeGetPlane(drm_fd, plane_id);
        ALOGE("[zizhan] INFO, plane[%d]: plane-id: %d crtc-id: %d fb-id: %d", 
        i, plane->plane_id, plane->crtc_id, plane->fb_id);
        if (plane->crtc_id == crtc_id) {
            break;
        }
    }
    ALOGE("[zizhan] after loop, plane: plane-id: %d crtc-id: %d fb-id: %d", 
        plane->plane_id, plane->crtc_id, plane->fb_id);
	// This test for only 1 plane
	

	ALOGE("INFO: connector->name:%s-%u\n", util_lookup_connector_type_name(conn->connector_type),
		conn->connector_type_id);
	ALOGE("INFO: connector id = %d / crtc id = %d / plane id = %d\n",
		connector_id, crtc_id, plane_id);
//milestone

	//Start Mode Setting
	struct bo *bo;
	drmModeModeInfo *mode;
	// Max planes:4
	// handles: dumb buffer handle, create by libdrm api
	// pitches: real linesize, by Bytes.
	// offset: every plane offset.
	unsigned int handles[4] = {0};
	unsigned int pitches[4] = {0};
	unsigned int offsets[4] = {0};
	uint64_t cap = 0;

	// Support dumb buffer?
	ret = drmGetCap(drm_fd, DRM_CAP_DUMB_BUFFER, &cap);
	if (ret || cap == 0) {
		ALOGE("ERROR: driver doesn't support the dumb buffer API\n");
		//goto OUT;
        return -1;
	}

	// Create dumb buffer and mmap it
	bo = bo_create(drm_fd, DRM_FORMAT_RGB888, width, height, handles, pitches, offsets, UTIL_PATTERN_SMPTE);
	if (bo == NULL) {
		ALOGE("ERROR: bo_create failed!\n");
		//goto OUT;
        return -1;
	}

	for (int i = 0; i < 4; i++)
		ALOGE("INFO: #%d handles:%d, pitches:%d, offsets:%d\n", i, handles[i], pitches[i], offsets[i]);

	// Fill date: full screen is white
	memset(bo->ptr, 255, height * pitches[0]);

	// Get framebuff id, this id is used by drm api to find the buffer.
	// Format the display memory, then the drm api knows what format to
	// display the memory of the drm application.
	//
	ret = drmModeAddFB2(drm_fd, width, height, DRM_FORMAT_RGB888, handles, pitches, offsets, &fb_id, 0);
	if (ret) {
		ALOGE("ERROR: drmModeAddFB2 failed!\n");
		//goto OUT;
        return -1;
	}
//milestone


	// Get the crtc display mode. For this test, only 1 mode support.
	mode = &(conn->modes[0]);
    ALOGE("[zizhan] INFO: connector mode count:%d", conn->count_modes);
    if (mode == NULL) {
        ALOGE("[zizhan] ERROR: mode is NULL!\n");
        return -1;
    }
	ALOGE("[zizhan] INFO: mode->mame:%s, %dx%d, Conn_id:%d, crtc_id:%d\n",
		mode->name, mode->hdisplay, mode->vdisplay, connector_id, crtc_id);
//milestone

	ret = drmModeSetCrtc(drm_fd, crtc_id, fb_id, 0, 0, (uint32_t *)&connector_id, 1, mode);
	if (ret) {
		ALOGE("ERROR: drmModeSetCrtc failed!\n");
		//goto OUT;
        return -1;
	}

	// XXX: Actually check if this is needed
	drmModeDirtyFB(drm_fd, fb_id, NULL, 0);

	// show modeseting effect: full screen white 
	sleep(1);

	// Start Set plane: show frame to plane
	while (frame_cnt--) {
	    //fill_frame_rgb24_date((unsigned char *)bo->ptr, width, height, pitches[0]);
        memcpy(bo->ptr, ptr, height * pitches[0]);
		//debug_save_file(bo->ptr, pitches[0]*height);

		// note src coords (last 4 args) are in Q16 format
        if (drmModeSetPlane(drm_fd, plane_id, crtc_id, fb_id,
		    0, 0, 0, width, height, 0, 0, width << 16, height << 16)) {
		    ALOGE("[zizhan] ERROR: drmModeSetPlane failed! plane_id:%d, fb_id:%d\n", plane_id, fb_id);
		    return -1;
	    }
		sleep(1);
	}

//OUT:

	if (bo) {
		if (bo->ptr)
			drm_munmap(bo->ptr, bo->size);
		bo->ptr = NULL;
		bo_destroy(bo);
	}

	if (pres)
		drmModeFreePlaneResources (pres);
	if (conn)
		drmModeFreeConnector (conn);
	if (res)
		drmModeFreeResources (res);

	if (drm_fd)
		drmClose(drm_fd);

    return 0;
}

static jlong nativeLockCanvas(JNIEnv* env, jclass clazz,
        jlong nativeObject, jobject canvasObj, jobject dirtyRectObj) {
    sp<Surface> surface(reinterpret_cast<Surface *>(nativeObject));

    if (!isSurfaceValid(surface)) {
        doThrowIAE(env);
        return 0;
    }

    if (convertPixelFormat(ANativeWindow_getFormat(surface.get())) == kUnknown_SkColorType) {
        native_window_set_buffers_format(surface.get(), PIXEL_FORMAT_RGBA_8888);
    }

    Rect dirtyRect(Rect::EMPTY_RECT);
    Rect* dirtyRectPtr = NULL;

    if (dirtyRectObj) {
        dirtyRect.left   = env->GetIntField(dirtyRectObj, gRectClassInfo.left);
        dirtyRect.top    = env->GetIntField(dirtyRectObj, gRectClassInfo.top);
        dirtyRect.right  = env->GetIntField(dirtyRectObj, gRectClassInfo.right);
        dirtyRect.bottom = env->GetIntField(dirtyRectObj, gRectClassInfo.bottom);
        dirtyRectPtr = &dirtyRect;
    }

//zizhan
    ANativeWindow_Buffer outBuffer;
    status_t err = surface->lock(&outBuffer, dirtyRectPtr);
    if (err < 0) {
        const char* const exception = (err == NO_MEMORY) ?
                OutOfResourcesException :
                "java/lang/IllegalArgumentException";
        jniThrowException(env, exception, NULL);
        return 0;
    }
    drm_display_zizhan(outBuffer.bits);

    SkImageInfo info = SkImageInfo::Make(outBuffer.width, outBuffer.height,
                                         convertPixelFormat(outBuffer.format),
                                         outBuffer.format == PIXEL_FORMAT_RGBX_8888
                                                 ? kOpaque_SkAlphaType : kPremul_SkAlphaType);

    SkBitmap bitmap;
    ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
    bitmap.setInfo(info, bpr);
    if (outBuffer.width > 0 && outBuffer.height > 0) {
        bitmap.setPixels(outBuffer.bits);
    } else {
        // be safe with an empty bitmap.
        bitmap.setPixels(NULL);
    }

    Canvas* nativeCanvas = GraphicsJNI::getNativeCanvas(env, canvasObj);
    nativeCanvas->setBitmap(bitmap);

    if (dirtyRectPtr) {
        nativeCanvas->clipRect(dirtyRect.left, dirtyRect.top,
                dirtyRect.right, dirtyRect.bottom, SkClipOp::kIntersect);
    }

    if (dirtyRectObj) {
        env->SetIntField(dirtyRectObj, gRectClassInfo.left,   dirtyRect.left);
        env->SetIntField(dirtyRectObj, gRectClassInfo.top,    dirtyRect.top);
        env->SetIntField(dirtyRectObj, gRectClassInfo.right,  dirtyRect.right);
        env->SetIntField(dirtyRectObj, gRectClassInfo.bottom, dirtyRect.bottom);
    }

    // Create another reference to the surface and return it.  This reference
    // should be passed to nativeUnlockCanvasAndPost in place of mNativeObject,
    // because the latter could be replaced while the surface is locked.
    sp<Surface> lockedSurface(surface);
    lockedSurface->incStrong(&sRefBaseOwner);
    return (jlong) lockedSurface.get();
}

//zizhan
static void nativeUnlockCanvasAndPost(JNIEnv* env, jclass clazz,
        jlong nativeObject, jobject canvasObj) {
    sp<Surface> surface(reinterpret_cast<Surface *>(nativeObject));
    if (!isSurfaceValid(surface)) {
        return;
    }

    // detach the canvas from the surface
    Canvas* nativeCanvas = GraphicsJNI::getNativeCanvas(env, canvasObj);
    nativeCanvas->setBitmap(SkBitmap());

    // unlock surface
    status_t err = surface->unlockAndPost();
    if (err < 0) {
        doThrowIAE(env);
    }
    //test_zizhan();
}

static void nativeAllocateBuffers(JNIEnv* /* env */ , jclass /* clazz */,
        jlong nativeObject) {
    sp<Surface> surface(reinterpret_cast<Surface *>(nativeObject));
    if (!isSurfaceValid(surface)) {
        return;
    }

    surface->allocateBuffers();
}

// ----------------------------------------------------------------------------

static jlong nativeCreateFromSurfaceControl(JNIEnv* env, jclass clazz,
        jlong surfaceControlNativeObj) {
    sp<SurfaceControl> ctrl(reinterpret_cast<SurfaceControl *>(surfaceControlNativeObj));
    sp<Surface> surface(ctrl->createSurface());
    if (surface != NULL) {
        surface->incStrong(&sRefBaseOwner);
    }
    return reinterpret_cast<jlong>(surface.get());
}

static jlong nativeGetFromSurfaceControl(JNIEnv* env, jclass clazz,
        jlong nativeObject,
        jlong surfaceControlNativeObj) {
    Surface* self(reinterpret_cast<Surface *>(nativeObject));
    sp<SurfaceControl> ctrl(reinterpret_cast<SurfaceControl *>(surfaceControlNativeObj));

    // If the underlying IGBP's are the same, we don't need to do anything.
    if (self != nullptr &&
            IInterface::asBinder(self->getIGraphicBufferProducer()) ==
            IInterface::asBinder(ctrl->getIGraphicBufferProducer())) {
        return nativeObject;
    }

    sp<Surface> surface(ctrl->getSurface());
    if (surface != NULL) {
        surface->incStrong(&sRefBaseOwner);
    }

    return reinterpret_cast<jlong>(surface.get());
}

static jlong nativeReadFromParcel(JNIEnv* env, jclass clazz,
        jlong nativeObject, jobject parcelObj) {
    Parcel* parcel = parcelForJavaObject(env, parcelObj);
    if (parcel == NULL) {
        doThrowNPE(env);
        return 0;
    }

    android::view::Surface surfaceShim;

    // Calling code in Surface.java has already read the name of the Surface
    // from the Parcel
    surfaceShim.readFromParcel(parcel, /*nameAlreadyRead*/true);

    sp<Surface> self(reinterpret_cast<Surface *>(nativeObject));

    // update the Surface only if the underlying IGraphicBufferProducer
    // has changed.
    if (self != nullptr
            && (IInterface::asBinder(self->getIGraphicBufferProducer()) ==
                    IInterface::asBinder(surfaceShim.graphicBufferProducer))) {
        // same IGraphicBufferProducer, return ourselves
        return jlong(self.get());
    }

    sp<Surface> sur;
    if (surfaceShim.graphicBufferProducer != nullptr) {
        // we have a new IGraphicBufferProducer, create a new Surface for it
        sur = new Surface(surfaceShim.graphicBufferProducer, true);
        // and keep a reference before passing to java
        sur->incStrong(&sRefBaseOwner);
    }

    if (self != NULL) {
        // and loose the java reference to ourselves
        self->decStrong(&sRefBaseOwner);
    }

    return jlong(sur.get());
}

static void nativeWriteToParcel(JNIEnv* env, jclass clazz,
        jlong nativeObject, jobject parcelObj) {
    Parcel* parcel = parcelForJavaObject(env, parcelObj);
    if (parcel == NULL) {
        doThrowNPE(env);
        return;
    }
    sp<Surface> self(reinterpret_cast<Surface *>(nativeObject));
    android::view::Surface surfaceShim;
    if (self != nullptr) {
        surfaceShim.graphicBufferProducer = self->getIGraphicBufferProducer();
    }
    // Calling code in Surface.java has already written the name of the Surface
    // to the Parcel
    surfaceShim.writeToParcel(parcel, /*nameAlreadyWritten*/true);
}

static jint nativeGetWidth(JNIEnv* env, jclass clazz, jlong nativeObject) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    ANativeWindow* anw = static_cast<ANativeWindow*>(surface);
    int value = 0;
    anw->query(anw, NATIVE_WINDOW_WIDTH, &value);
    return value;
}

static jint nativeGetHeight(JNIEnv* env, jclass clazz, jlong nativeObject) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    ANativeWindow* anw = static_cast<ANativeWindow*>(surface);
    int value = 0;
    anw->query(anw, NATIVE_WINDOW_HEIGHT, &value);
    return value;
}

static jlong nativeGetNextFrameNumber(JNIEnv *env, jclass clazz, jlong nativeObject) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    return surface->getNextFrameNumber();
}

static jint nativeSetScalingMode(JNIEnv *env, jclass clazz, jlong nativeObject, jint scalingMode) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    return surface->setScalingMode(scalingMode);
}

static jint nativeForceScopedDisconnect(JNIEnv *env, jclass clazz, jlong nativeObject) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    return surface->disconnect(-1, IGraphicBufferProducer::DisconnectMode::AllLocal);
}

static jint nativeAttachAndQueueBufferWithColorSpace(JNIEnv *env, jclass clazz, jlong nativeObject,
        jobject graphicBuffer, jint colorSpaceId) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    sp<GraphicBuffer> bp = graphicBufferForJavaObject(env, graphicBuffer);
    int err = Surface::attachAndQueueBufferWithDataspace(surface, bp,
            fromNamedColorSpaceValueToDataspace(colorSpaceId));
    return err;
}

static jint nativeSetSharedBufferModeEnabled(JNIEnv* env, jclass clazz, jlong nativeObject,
        jboolean enabled) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    ANativeWindow* anw = static_cast<ANativeWindow*>(surface);
    return anw->perform(surface, NATIVE_WINDOW_SET_SHARED_BUFFER_MODE, int(enabled));
}

static jint nativeSetAutoRefreshEnabled(JNIEnv* env, jclass clazz, jlong nativeObject,
        jboolean enabled) {
    Surface* surface = reinterpret_cast<Surface*>(nativeObject);
    ANativeWindow* anw = static_cast<ANativeWindow*>(surface);
    return anw->perform(surface, NATIVE_WINDOW_SET_AUTO_REFRESH, int(enabled));
}

namespace uirenderer {

using namespace android::uirenderer::renderthread;

class ContextFactory : public IContextFactory {
public:
    virtual AnimationContext* createAnimationContext(renderthread::TimeLord& clock) {
        return new AnimationContext(clock);
    }
};

static jlong create(JNIEnv* env, jclass clazz, jlong rootNodePtr, jlong surfacePtr,
        jboolean isWideColorGamut) {
    RenderNode* rootNode = reinterpret_cast<RenderNode*>(rootNodePtr);
    sp<Surface> surface(reinterpret_cast<Surface*>(surfacePtr));
    ContextFactory factory;
    RenderProxy* proxy = new RenderProxy(false, rootNode, &factory);
    proxy->loadSystemProperties();
    if (isWideColorGamut) {
        proxy->setWideGamut(true);
    }
    proxy->setSwapBehavior(SwapBehavior::kSwap_discardBuffer);
    proxy->setSurface(surface, false);
    // Shadows can't be used via this interface, so just set the light source
    // to all 0s.
    proxy->setLightAlpha(0, 0);
    proxy->setLightGeometry((Vector3){0, 0, 0}, 0);
    return (jlong) proxy;
}

static void setSurface(JNIEnv* env, jclass clazz, jlong rendererPtr, jlong surfacePtr) {
    RenderProxy* proxy = reinterpret_cast<RenderProxy*>(rendererPtr);
    sp<Surface> surface(reinterpret_cast<Surface*>(surfacePtr));
    proxy->setSurface(surface);
}

static void draw(JNIEnv* env, jclass clazz, jlong rendererPtr) {
    RenderProxy* proxy = reinterpret_cast<RenderProxy*>(rendererPtr);
    nsecs_t vsync = systemTime(CLOCK_MONOTONIC);
    UiFrameInfoBuilder(proxy->frameInfo())
            .setVsync(vsync, vsync)
            .addFlag(FrameInfoFlags::SurfaceCanvas);
    proxy->syncAndDrawFrame();
}

static void destroy(JNIEnv* env, jclass clazz, jlong rendererPtr) {
    RenderProxy* proxy = reinterpret_cast<RenderProxy*>(rendererPtr);
    delete proxy;
}

} // uirenderer

// ----------------------------------------------------------------------------

namespace hwui = android::uirenderer;

static const JNINativeMethod gSurfaceMethods[] = {
    {"nativeCreateFromSurfaceTexture", "(Landroid/graphics/SurfaceTexture;)J",
            (void*)nativeCreateFromSurfaceTexture },
    {"nativeRelease", "(J)V",
            (void*)nativeRelease },
    {"nativeIsValid", "(J)Z",
            (void*)nativeIsValid },
    {"nativeIsConsumerRunningBehind", "(J)Z",
            (void*)nativeIsConsumerRunningBehind },
    {"nativeLockCanvas", "(JLandroid/graphics/Canvas;Landroid/graphics/Rect;)J",
            (void*)nativeLockCanvas },
    {"nativeUnlockCanvasAndPost", "(JLandroid/graphics/Canvas;)V",
            (void*)nativeUnlockCanvasAndPost },
    {"nativeAllocateBuffers", "(J)V",
            (void*)nativeAllocateBuffers },
    {"nativeCreateFromSurfaceControl", "(J)J",
            (void*)nativeCreateFromSurfaceControl },
    {"nativeGetFromSurfaceControl", "(JJ)J",
            (void*)nativeGetFromSurfaceControl },
    {"nativeReadFromParcel", "(JLandroid/os/Parcel;)J",
            (void*)nativeReadFromParcel },
    {"nativeWriteToParcel", "(JLandroid/os/Parcel;)V",
            (void*)nativeWriteToParcel },
    {"nativeGetWidth", "(J)I", (void*)nativeGetWidth },
    {"nativeGetHeight", "(J)I", (void*)nativeGetHeight },
    {"nativeGetNextFrameNumber", "(J)J", (void*)nativeGetNextFrameNumber },
    {"nativeSetScalingMode", "(JI)I", (void*)nativeSetScalingMode },
    {"nativeForceScopedDisconnect", "(J)I", (void*)nativeForceScopedDisconnect},
    {"nativeAttachAndQueueBufferWithColorSpace", "(JLandroid/graphics/GraphicBuffer;I)I",
            (void*)nativeAttachAndQueueBufferWithColorSpace},
    {"nativeSetSharedBufferModeEnabled", "(JZ)I", (void*)nativeSetSharedBufferModeEnabled},
    {"nativeSetAutoRefreshEnabled", "(JZ)I", (void*)nativeSetAutoRefreshEnabled},

    // HWUI context
    {"nHwuiCreate", "(JJZ)J", (void*) hwui::create },
    {"nHwuiSetSurface", "(JJ)V", (void*) hwui::setSurface },
    {"nHwuiDraw", "(J)V", (void*) hwui::draw },
    {"nHwuiDestroy", "(J)V", (void*) hwui::destroy },
};

int register_android_view_Surface(JNIEnv* env)
{
    int err = RegisterMethodsOrDie(env, "android/view/Surface",
            gSurfaceMethods, NELEM(gSurfaceMethods));

    jclass clazz = FindClassOrDie(env, "android/view/Surface");
    gSurfaceClassInfo.clazz = MakeGlobalRefOrDie(env, clazz);
    gSurfaceClassInfo.mNativeObject = GetFieldIDOrDie(env,
            gSurfaceClassInfo.clazz, "mNativeObject", "J");
    gSurfaceClassInfo.mLock = GetFieldIDOrDie(env,
            gSurfaceClassInfo.clazz, "mLock", "Ljava/lang/Object;");
    gSurfaceClassInfo.ctor = GetMethodIDOrDie(env, gSurfaceClassInfo.clazz, "<init>", "(J)V");

    clazz = FindClassOrDie(env, "android/graphics/Rect");
    gRectClassInfo.left = GetFieldIDOrDie(env, clazz, "left", "I");
    gRectClassInfo.top = GetFieldIDOrDie(env, clazz, "top", "I");
    gRectClassInfo.right = GetFieldIDOrDie(env, clazz, "right", "I");
    gRectClassInfo.bottom = GetFieldIDOrDie(env, clazz, "bottom", "I");

    return err;
}

};
