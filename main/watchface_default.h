#pragma once

#include "watchface.h"

class DefaultWatchface : public Watchface {
  using Watchface::Watchface; // Forward the constructor

  // Needs to implement minute uni/dec draw & return Rect coordinates
  void drawU(uint8_t d) override;
  void drawD(uint8_t d) override;
  Rect rectU() override {return {150,20,38,53}; }
  Rect rectD() override {return {106,20,38,53}; }

  // Can optionally implement Other element drawing based & return vect of rect
  std::vector<Rect> render() override;


  void drawEllipse(int x, int y, uint8_t width, uint8_t height, uint16_t on);
  void drawEllipseDifference(int x, int y, uint8_t width1, uint8_t width2, uint8_t height, bool big, uint16_t color);
  void drawMoonFast(float p, int x, int y, uint8_t r, uint16_t on, uint16_t off);
  void drawMoon(float p, uint16_t x, uint16_t y, uint16_t radius, uint16_t on, uint16_t off);
};