
#ifndef THREADS_THUMBNAIL_THREAD_HPP_
#define THREADS_THUMBNAIL_THREAD_HPP_

#include <algorithm>
#include <atomic>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "loadbmp.hpp"
#include "screenshots.hpp"
#include "ui.hpp"

namespace screenshots::threads {

class ThumbnailThread {
   private:
    struct ThumbnailCache {
        C2D_Image image;
        size_t last_usage = 0;
        mutable_info_ptr assigned_screenshot;

        explicit ThumbnailCache(mutable_info_ptr info) : assigned_screenshot(info) { image = ui::CreateImage(ui::kThumbnailWidth, ui::kThumbnailHeight); }
        ThumbnailCache(const ThumbnailCache &) = delete;
        ThumbnailCache &operator=(const ThumbnailCache &other) = delete;
        ThumbnailCache &operator=(const ThumbnailCache &&other) = delete;

        ~ThumbnailCache() {
            if (image.tex != nullptr || image.subtex != nullptr) {
                C3D_TexDelete(image.tex);
                delete image.tex;
                delete image.subtex;
            }
        }
        ThumbnailCache(ThumbnailCache &&other) : image(std::move(other.image)), last_usage(other.last_usage), assigned_screenshot(other.assigned_screenshot) {
            other.image.tex = nullptr;
            other.image.subtex = nullptr;
        }
    };

    static constexpr size_t kThumbnailsPerPage = 9;

    // Number of thumbnails to load around the thumbnail_cache_iterator screenshot
    static constexpr size_t kCacheRange = kThumbnailsPerPage * 15;
    // Distance from thumbnail_cache_iterator that a thumbnail requested with "Load" must be to replace the thumbnail_cache_iterator with it
    static constexpr size_t kCacheBoundary = kThumbnailsPerPage * 5;

    /*
     * (cache_iter - kCacheRange) === (cache_iter - kCacheBoundary) === cache_iter === (cache_iter + kCacheBoundary) === (cache_iter + kCacheRange)
     *             ^                              ^                                                ^                                 ^
     *             |                              |________________________________________________|                                 |
     *             |      "SetCurrent" calls outside this range replaces cache_iter and starts loading thumbnails from there         |
     *             |                                                                                                                 |
     *             |_________________________________________________________________________________________________________________|
     *        "ThreadMain" load thumbnails in this range, starting from cache_iter and alternating between screenshots in the neightborhood
     *
     */

    // Max cache size
    static constexpr size_t kMaxThumbnails = std::max(kCacheRange * 2 + 1, 1000U);

    using mutable_info_ptr_iterator = std::vector<screenshots::mutable_info_ptr>::iterator;

    mutable_info_ptr_iterator screenshot_container_start;
    mutable_info_ptr_iterator screenshot_container_end;

    std::list<ThumbnailCache> thumbnails_cache;
    std::map<mutable_info_ptr, std::list<ThumbnailCache>::iterator> screenshots_in_cache;
    std::atomic<size_t> thumbnail_cache_tick = 0;

    std::vector<screenshots::mutable_info_ptr>::iterator thumbnail_cache_iterator;

    std::atomic<bool> loading_thumbnails = false;
    std::atomic<bool> run_thread = false;
    std::atomic<int> loaded_thumbs = 0;

    Thread thumbnailThread;
    Handle loadThumbnailRequest;

    void LoadThumbnail(mutable_info_ptr info) {
        if (screenshots_in_cache.contains(info)) {
            screenshots_in_cache[info]->last_usage = thumbnail_cache_tick;
            thumbnails_cache.splice(thumbnails_cache.end(), thumbnails_cache, screenshots_in_cache[info]);
            return;
        }

        if (thumbnails_cache.size() < kMaxThumbnails) {
            thumbnails_cache.push_back(ThumbnailCache(info));
        } else {
            ThumbnailCache oldest_thumbnail = std::move(thumbnails_cache.front());
            thumbnails_cache.pop_front();

            oldest_thumbnail.assigned_screenshot->thumbnail = nullptr;
            oldest_thumbnail.assigned_screenshot->has_thumbnail = false;
            screenshots_in_cache.erase(oldest_thumbnail.assigned_screenshot);

            thumbnails_cache.push_back(std::move(oldest_thumbnail));
        }
        ThumbnailCache *thumbnail = &thumbnails_cache.back();

        info->has_thumbnail = false;
        unsigned int error = loadbmp_to_image(info->path_top, thumbnail->image);
        info->thumbnail = &thumbnail->image;
        info->has_thumbnail = !error;

        thumbnail->last_usage = thumbnail_cache_tick;
        thumbnail->assigned_screenshot = info;
        screenshots_in_cache[info] = --thumbnails_cache.end();

        loaded_thumbs = loaded_thumbs + 1;
    }

    void ThreadMain() {
        while (run_thread) {
            if (thumbnail_cache_tick != 0) {
                loading_thumbnails = true;

                auto cache_iterator = thumbnail_cache_iterator;
                for (size_t i = 0; i < kCacheRange; i++) {
                    if (!run_thread) {
                        return;
                    }

                    if (cache_iterator == thumbnail_cache_iterator) {
                        // Skip thumbnail requests to the current cache_iterator
                        svcClearEvent(loadThumbnailRequest);
                    } else {
                        // Break and start loading from the new iterator
                        break;
                    }

                    if (cache_iterator + i < screenshot_container_end) {
                        LoadThumbnail(*(cache_iterator + i));
                    }

                    size_t previous_page_offset =
                        i > 0 ? ((i - 1) / kThumbnailsPerPage) * kThumbnailsPerPage + (kThumbnailsPerPage - ((i - 1) % kThumbnailsPerPage)) : 0;

                    if (cache_iterator - previous_page_offset >= screenshot_container_start && i != 0) {
                        LoadThumbnail(*(cache_iterator - previous_page_offset));
                    }
                }
                loading_thumbnails = false;
            }

            svcWaitSynchronization(loadThumbnailRequest, U64_MAX);
            svcClearEvent(loadThumbnailRequest);
        }
    }

    static void ThreadEntrypointFn(void *arg) {
        ThumbnailThread &thread = *static_cast<ThumbnailThread *>(arg);
        thread.ThreadMain();
    }

   public:
    explicit ThumbnailThread(mutable_info_ptr_iterator screenshot_container_start, mutable_info_ptr_iterator screenshot_container_end) {
        Start(screenshot_container_start, screenshot_container_end);
    }

    ~ThumbnailThread() { Stop(); }

    size_t NumLoadedThumbnails() { return loaded_thumbs; }

    void SetCurrent(mutable_info_ptr_iterator new_current_iterator) {
        if (screenshot_container_start == screenshot_container_end) return;

        if (thumbnail_cache_tick != 0) {
            size_t distance = std::abs(std::distance(thumbnail_cache_iterator, new_current_iterator));

            if (distance < kCacheBoundary) {
                return;
            }
        }

        thumbnail_cache_iterator = new_current_iterator;
        thumbnail_cache_tick++;
        svcSignalEvent(loadThumbnailRequest);
    }

    void Stop() {
        if (!run_thread) return;

        run_thread = false;

        svcClearEvent(loadThumbnailRequest);
        svcSignalEvent(loadThumbnailRequest);

        threadJoin(thumbnailThread, U64_MAX);
        threadFree(thumbnailThread);

        svcCloseHandle(loadThumbnailRequest);
    }

    void Start(mutable_info_ptr_iterator screenshot_container_start, mutable_info_ptr_iterator screenshot_container_end) {
        if (run_thread) return;

        this->screenshot_container_start = screenshot_container_start;
        this->screenshot_container_end = screenshot_container_end;

        thumbnail_cache_tick = 0;

        s32 prio = 0;
        svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
        svcCreateEvent(&loadThumbnailRequest, RESET_ONESHOT);
        run_thread = true;

        size_t stackSize = (4 * 1024);
        thumbnailThread = threadCreate(ThreadEntrypointFn, this, stackSize, prio - 1, -2, false);
    }
};
}  // namespace screenshots::threads

#endif  // THREADS_THUMBNAIL_THREAD_HPP_
