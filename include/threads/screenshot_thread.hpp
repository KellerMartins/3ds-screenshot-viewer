#ifndef THREADS_SCREENSHOT_THREAD_HPP_
#define THREADS_SCREENSHOT_THREAD_HPP_

#include <3ds.h>

#include <atomic>
#include <iostream>
#include <vector>

#include "loadbmp.hpp"
#include "screenshots.hpp"
#include "ui.hpp"

namespace screenshots::threads {

class ScreenshotThread {
   private:
    std::atomic<bool> run_thread = false;
    std::atomic<bool> loading_screenshot = false;

    info_ptr loading_screenshot_info = nullptr;
    void (*loading_screenshot_callback)(screenshot_ptr) = nullptr;

    std::atomic<info_ptr> next_screenshot_info = nullptr;
    void (*next_screenshot_callback)(screenshot_ptr) = nullptr;

    // Use a screenshot for loading and another for returning to the callback
    Screenshot *screenshot_buffer[2];
    static constexpr int num_buffers = (sizeof(screenshot_buffer) / sizeof(screenshot_buffer[0]));

    int current_buffer = 0;
    int last_buffer = 0;

    Thread loadScreenshotThread;
    Handle loadScreenshotRequest;

    void LoadScreenshot(info_ptr screenshot_info) {
        Screenshot *screenshot = screenshot_buffer[current_buffer];

        unsigned int error;
        if (screenshot_info->path_top_right.size() > 0) {
            error = loadbmp_to_texture(screenshot_info->path_top_right, screenshot->top_right.tex);
            if (error) {
                screenshot->is_3d = false;
            } else {
                screenshot->is_3d = true;
            }
        } else {
            screenshot->is_3d = false;
        }

        error = loadbmp_to_texture(screenshot_info->path_top, screenshot->top.tex);
        if (error) {
            memset(screenshot->top_right.tex->data, 0, screenshot->top_right.tex->size);
            memset(screenshot->top.tex->data, 0, screenshot->top.tex->size);
            screenshot->is_3d = false;
        }

        error = loadbmp_to_texture(screenshot_info->path_bottom, screenshot->bottom.tex);
        if (error) {
            memset(screenshot->bottom.tex->data, 0, screenshot->bottom.tex->size);
        }
    }

    void ThreadMain() {
        while (run_thread) {
            auto next_info = next_screenshot_info.load();

            if (next_info != nullptr) {
                if (next_info != loading_screenshot_info) {
                    loading_screenshot_info = next_info;
                    loading_screenshot_callback = next_screenshot_callback;

                    LoadScreenshot(loading_screenshot_info);

                    loading_screenshot_callback(screenshot_buffer[current_buffer]);

                    last_buffer = current_buffer;
                    current_buffer = (current_buffer + 1) % num_buffers;
                } else {
                    loading_screenshot_callback(screenshot_buffer[last_buffer]);
                }
            }

            svcWaitSynchronization(loadScreenshotRequest, U64_MAX);
            svcClearEvent(loadScreenshotRequest);
        }
    }

    static void ThreadEntrypointFn(void *arg) {
        ScreenshotThread &thread = *static_cast<ScreenshotThread *>(arg);
        thread.ThreadMain();
    }

   public:
    ScreenshotThread() {
        for (int i = 0; i < num_buffers; i++) {
            screenshot_buffer[i] = new Screenshot({
                false,
                ui::CreateImage(ui::kTopScreenWidth, ui::kTopScreenHeight),
                ui::CreateImage(ui::kTopScreenWidth, ui::kTopScreenHeight),
                ui::CreateImage(ui::kBottomScreenWidth, ui::kBottomScreenHeight),
            });
        }

        Start();
    }

    ~ScreenshotThread() { Stop(); }

    void Load(info_ptr screenshot_info, void (*callback)(screenshot_ptr)) {
        if (screenshot_info == nullptr) {
            callback(nullptr);
            return;
        }

        next_screenshot_info = screenshot_info;
        next_screenshot_callback = callback;

        svcClearEvent(loadScreenshotRequest);
        svcSignalEvent(loadScreenshotRequest);
    }

    void Stop() {
        if (!run_thread) return;

        run_thread = false;

        svcClearEvent(loadScreenshotRequest);
        svcSignalEvent(loadScreenshotRequest);
        threadJoin(loadScreenshotThread, U64_MAX);
        threadFree(loadScreenshotThread);
        svcCloseHandle(loadScreenshotRequest);
    }

    void Start() {
        if (run_thread) return;

        s32 prio = 0;
        svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
        svcCreateEvent(&loadScreenshotRequest, RESET_ONESHOT);
        run_thread = true;

        size_t stackSize = (4 * 1024);
        loadScreenshotThread = threadCreate(ThreadEntrypointFn, this, stackSize, prio - 1, -2, false);
    }
};
}  // namespace screenshots::threads

#endif  // THREADS_SCREENSHOT_THREAD_HPP_
