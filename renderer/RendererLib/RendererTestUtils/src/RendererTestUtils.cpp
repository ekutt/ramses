//  -------------------------------------------------------------------------
//  Copyright (C) 2014 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "RendererTestUtils.h"
#include "ramses-renderer-api/RamsesRenderer.h"
#include "ramses-renderer-api/WarpingMeshData.h"
#include "ramses-renderer-api/IRendererEventHandler.h"
#include "RamsesRendererImpl.h"
#include "DisplayConfigImpl.h"
#include "RendererConfigImpl.h"
#include "Utils/Image.h"
#include "Utils/File.h"
#include "Collections/String.h"
#include "RendererAndSceneTestEventHandler.h"
#include "RendererAPI/IRenderBackend.h"
#include "RendererAPI/ISurface.h"
#include "ReadPixelCallbackHandler.h"

using namespace ramses_internal;

const float RendererTestUtils::DefaultMaxAveragePercentPerPixel = 0.2f;
std::chrono::microseconds RendererTestUtils::MaxFrameCallbackPollingTime;
ramses_internal::String RendererTestUtils::WaylandDisplayForSystemCompositorController;
std::vector<const char*> RendererTestUtils::CommandLineArgs{};

namespace
{
    // If multiple platform backends are created at the same time, they have to have different id's
    // The numbers have special meaning, therefore the test uses 100, 101, 102, ... to make sure they
    // don't clash with existing applications
    const uint32_t firstIviSurfaceId = 10010;
}

void RendererTestUtils::SaveScreenshotForDisplay(ramses::RamsesRenderer& renderer, ramses::displayId_t displayId, UInt32 x, UInt32 y, UInt32 width, UInt32 height, const String& screenshotFileName)
{
    const Image screenshotBitmap = ReadPixelData(renderer, displayId, x, y, width, height);
    screenshotBitmap.saveToFilePNG("./res/" + screenshotFileName + ".PNG");
}

bool RendererTestUtils::PerformScreenshotTestForDisplay(ramses::RamsesRenderer& renderer, ramses::displayId_t displayId, UInt32 x, UInt32 y, UInt32 width, UInt32 height, const String& screenshotFileName, float maxAveragePercentErrorPerPixel)
{
    const Image screenshotBitmap = ReadPixelData(renderer, displayId, x, y, width, height);
    return CompareBitmapToImageInFile(screenshotBitmap, screenshotFileName, maxAveragePercentErrorPerPixel);
}

bool RendererTestUtils::CompareBitmapToImageInFile(const Image& actualBitmap, const String& expectedScreenshotFileName, float maxAveragePercentErrorPerPixel)
{
    Image expectedBitmap;
    expectedBitmap.loadFromFilePNG("./res/" + expectedScreenshotFileName + ".PNG");
    if (expectedBitmap.getWidth() != actualBitmap.getWidth() ||
        expectedBitmap.getHeight() != actualBitmap.getHeight())
    {
        printf("Screenshot comparison failed: size of expected image %u/%u does not match size of actual image %u/%u\n",
            expectedBitmap.getWidth(), expectedBitmap.getHeight(), actualBitmap.getWidth(), actualBitmap.getHeight());
        return false;
    }
    const Image diff = expectedBitmap.createDiffTo(actualBitmap);
    assert(diff.getNumberOfPixels() == actualBitmap.getNumberOfPixels());

    const UInt64 sumOfPixelDiff = diff.getSumOfPixelValues();
    const UInt32 totalNumberOfPixels = diff.getNumberOfPixels();

    if (totalNumberOfPixels == 0)
    {
        assert(false);
        printf("Screenshot comparison failed: difference bitmap has no pixels!\n");
        return false;
    }

    const Float averagePercentErrorPerPixel = (100.0f * sumOfPixelDiff / 256.0f) / totalNumberOfPixels;

    if (averagePercentErrorPerPixel > maxAveragePercentErrorPerPixel)
    {
        printf("Screenshot comparison failed: %s (%ux%u)\n", expectedScreenshotFileName.c_str(), diff.getWidth(), diff.getHeight());
        printf(" - avg error per pixel %.2f, maximum allowed error is %.2f\n", averagePercentErrorPerPixel, maxAveragePercentErrorPerPixel);
        printf(" - number of pixels different by more than 1 (one or more color channels): %u\n", diff.getNumberOfNonBlackPixels(1));
        printf(" - number of pixels different by more than 64 (one or more color channels): %u\n", diff.getNumberOfNonBlackPixels(64));
        printf(" - number of pixels different by more than 128 (one or more color channels): %u\n", diff.getNumberOfNonBlackPixels(128));
        printf(" - number of pixels different by 255 (one or more color channels): %u\n", diff.getNumberOfNonBlackPixels(254));

        const String screenShotPath = String("./res/diffs");

        File diffDirectory(screenShotPath);
        diffDirectory.createDirectory();

        actualBitmap.saveToFilePNG(screenShotPath + "/" + expectedScreenshotFileName + "_ACTUAL.PNG");
        diff.saveToFilePNG(screenShotPath + "/" + expectedScreenshotFileName + "_DIFF.PNG");

        return false;
    }

    return true;
}

ramses::displayId_t RendererTestUtils::CreateDisplayImmediate(ramses::RamsesRenderer& renderer, const ramses::DisplayConfig& displayConfig)
{
    const ramses::displayId_t displayId = renderer.createDisplay(displayConfig);
    renderer.flush();
    ramses::RendererAndSceneTestEventHandler eventHandler(renderer);
    if (eventHandler.waitForDisplayCreation(displayId))
    {
        return displayId;
    }

    assert(false && "Display construction failed or timed out!");
    return ramses::displayId_t::Invalid();
}

void RendererTestUtils::DestroyDisplayImmediate(ramses::RamsesRenderer& renderer, ramses::displayId_t displayId)
{
    renderer.destroyDisplay(displayId);
    renderer.flush();
    ramses::RendererAndSceneTestEventHandler eventHandler(renderer);
    if (!eventHandler.waitForDisplayDestruction(displayId))
    {
        assert(false && "Display destruction failed or timed out!");
    }
}

ramses::RendererConfig RendererTestUtils::CreateTestRendererConfig()
{
    ramses::RendererConfig rendererConfig(static_cast<int32_t>(CommandLineArgs.size()), CommandLineArgs.data());
    ramses_internal::RendererConfig& internalRendererConfig = const_cast<ramses_internal::RendererConfig&>(rendererConfig.impl.getInternalRendererConfig());

    if(WaylandDisplayForSystemCompositorController.size() > 0)
    {
        internalRendererConfig.enableSystemCompositorControl();
        internalRendererConfig.setWaylandDisplayForSystemCompositorController(WaylandDisplayForSystemCompositorController);
    }

    internalRendererConfig.setFrameCallbackMaxPollTime(MaxFrameCallbackPollingTime);

    return rendererConfig;
}

void RendererTestUtils::SetWaylandDisplayForSystemCompositorController(const ramses_internal::String& wd)
{
    WaylandDisplayForSystemCompositorController = wd;
}

void RendererTestUtils::SetCommandLineParams(const int argc, char const* const* argv)
{
    CommandLineArgs.assign(argv, argv + argc);
}

void RendererTestUtils::SetMaxFrameCallbackPollingTime(std::chrono::microseconds time)
{
    MaxFrameCallbackPollingTime = time;
}

ramses::DisplayConfig RendererTestUtils::CreateTestDisplayConfig(uint32_t iviSurfaceIdOffset, bool iviWindowStartVisible)
{
    ramses::DisplayConfig displayConfig(static_cast<int32_t>(CommandLineArgs.size()), CommandLineArgs.data());
    displayConfig.setWaylandIviSurfaceID(firstIviSurfaceId + iviSurfaceIdOffset);

    if(iviWindowStartVisible)
        displayConfig.setWindowIviVisible();

    return displayConfig;
}

const ramses::WarpingMeshData& RendererTestUtils::CreateTestWarpingMesh()
{
    const uint16_t indices[6] = { 0, 1, 2, 2, 1, 3 };
    const float trapezoidVertices[12] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -1.5f, 1.2f, 0.0f,
        1.5f, 1.2f, 0.0f
    };
    const float textureCoordinates[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    static const ramses::WarpingMeshData testWarpingMesh(6, indices, 4, trapezoidVertices, textureCoordinates);

    return testWarpingMesh;
}

Image RendererTestUtils::ReadPixelData(
    ramses::RamsesRenderer& renderer,
    ramses::displayId_t displayId,
    ramses_internal::UInt32 x,
    ramses_internal::UInt32 y,
    ramses_internal::UInt32 width,
    ramses_internal::UInt32 height)
{
    const ramses::status_t status = renderer.readPixels(displayId, {}, x, y, width, height);
    assert(status == ramses::StatusOK);
    UNUSED(status);
    renderer.flush();

    ReadPixelCallbackHandler callbackHandler;
    while (!callbackHandler.m_pixelDataRead)
    {
        if (renderer.impl.isThreaded())
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        else
            renderer.doOneLoop();
        renderer.dispatchEvents(callbackHandler);
    }

    assert(callbackHandler.m_pixelData.size() == width * height * 4u);

    // flip image vertically so that the layout read from frame buffer (bottom-up)
    // is converted to layout normally used in image files (top-down)
    return Image(width, height, callbackHandler.m_pixelData.cbegin(), callbackHandler.m_pixelData.cend(), true);
}
