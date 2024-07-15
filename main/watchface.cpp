#include "watchface.h"

void Watchface::updateCache() {
  auto& cache = mSettings.mWatchface.mCache;
  const auto& displ = mSettings.mWatchface.mCache;
  const auto& watch = mSettings.mWatchface;
  if (cache.mDone && displ.mRotation == cache.mRotation && watch.mType == cache.mType)
    return; // Cache is valid!
  
  auto fillCache = [&](Rect& r, uint8_t* data, bool units, size_t len){
    // Render all the cache elements, the spacing of elements is given by the Rect
    r = units ? rectU() : rectD();
    mDisplay.getAlignedRegion(r.x, r.y, r.w, r.h);
    auto size = r.size();
    for (auto d=0; d<len; d++) {
      if (units)
        drawU(d);
      else
        drawD(d);
      // Copy the area to the cache
      for(auto y=0; y<r.h; y++) {
        memcpy(
          data + size * d + y * r.w / 8,
          mDisplay.buffer + r.x/8 + (y + r.y) * mDisplay.WB_BITMAP,
          r.w / 8);
      }
      // Clear the area! // TODO optimize
      // mDisplay.fillRect(r.x, r.y, r.w, r.h, 0);
      mDisplay.fillScreen(0);
      // mDisplay.setTextColor(0);
    }
  };
  fillCache(cache.mUnits.coord, cache.mUnits.data, true, 10);
  fillCache(cache.mDecimal.coord, cache.mDecimal.data, false, 6);

  cache.mDone = true;
  cache.mRotation = displ.mRotation;
  cache.mType = watch.mType;
}

void Watchface::draw() {
  // Check & update the cache if needed
  updateCache();

  auto& last = mSettings.mWatchface.mLastDraw;

  auto copyCache2Display = [&](auto&& ptr, const Rect& r) {
    mDisplay.writeRegionAlignedPacked(ptr, r.x, r.y, r.w, r.h);
  };
  auto copyCache2Buffer = [&](auto&& ptr, const Rect& r) {
    for (auto y=0; y<r.h; y++)
      memcpy(
        mDisplay.buffer + r.x / 8 + (r.y + y) * mDisplay.WB_BITMAP,
        ptr + y * r.w / 8,
        r.w / 8 
      );
  };
  auto copyMinutes = [&]() {
    if (!last.mValid || last.mMinuteD != mNow.Minute / 10) {
      const auto& dec = mSettings.mWatchface.mCache.mDecimal;
      copyCache2Display(dec.data + dec.coord.size() * (mNow.Minute / 10), dec.coord);
    }
    if (!last.mValid || last.mMinuteU != mNow.Minute % 10) {
      const auto& units = mSettings.mWatchface.mCache.mUnits;
      copyCache2Display(units.data + units.coord.size() * (mNow.Minute % 10), units.coord);
    }
  };
  auto copyMinutes2Buffer = [&]() {
    {
      const auto& dec = mSettings.mWatchface.mCache.mDecimal;
      copyCache2Buffer(dec.data + dec.coord.size() * (mNow.Minute / 10), dec.coord);
    }
    {
      const auto& units = mSettings.mWatchface.mCache.mUnits;
      copyCache2Buffer(units.data + units.coord.size() * (mNow.Minute % 10), units.coord);
    }
  };

  // Common components
  auto composables = render();
  // ESP_LOGE("comp", "%d", composables.size());

  // Convert to aligned rotated coords, makes easier the copy
  for (auto& c : composables)      
    mDisplay.getAlignedRegion(c.x, c.y, c.w, c.h);

  // Update display / refresh
  if (!last.mValid){
    copyMinutes2Buffer();
    mDisplay.writeAllAndRefresh();
    mDisplay.writeAll(); // We need to write the other buffer for partial updates
  } else {
    // Pass 1: Copy all to display
    for (const auto& c : composables)      
      copyAlignedRectToDisplay(c);
    copyMinutes();

    // Manual refresh + swap buffers
    mDisplay.refresh();

    // Pass 3, copy again the updated parts to the other framebuffer
    for (const auto& c : composables)      
      copyAlignedRectToDisplay(c);
    copyMinutes();
  }

  // Store the values for next round
  last.mValid = true;
  last.mMinuteD = mNow.Minute / 10;
  last.mMinuteU = mNow.Minute % 10;
}