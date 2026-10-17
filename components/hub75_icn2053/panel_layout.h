// SPDX-FileCopyrightText: 2025 Stuart Parmenter
// SPDX-License-Identifier: MIT

// @file panel_layout.h
// @brief Multi-panel layout coordinate remapping

// Based on https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA

#pragma once

#include "layout_types.h"

namespace esphome::hub75_icn2053 {

// Multi-panel layout coordinate remapping
// Transforms virtual display coordinates to physical panel chain positions
//
// Virtual Display: What the user sees (e.g., 128×128 for a 2×2 grid)
// Physical Chain: How panels are wired in DMA (always horizontal: Panel 0 → 1 → 2 → ...)
//
// Row-major traversal: Panels chain HORIZONTALLY across rows
//
// Example (2×2 grid, TOP_LEFT_DOWN serpentine):
//   Virtual Display:        Panel Orientation:
//   ┌────┬────┐             ┌────┬────┐
//   │ 0  │ 1  │             │ 0→ │ ←1 │  (Panel 1 upside down)
//   ├────┼────┤             ├────┼────┤
//   │ 2  │ 3  │             │ 2→ │ ←3 │  (Panel 3 upside down)
//   └────┴────┘             └────┴────┘
//
//   Physical DMA chain: [Panel 0] → [Panel 1] → [Panel 2] → [Panel 3]
class PanelLayoutRemap {
 public:
  // Remap coordinates based on panel layout
  // @param c Input coordinates (virtual display space)
  // @param layout Layout type
  // @param panel_w Single panel width
  // @param panel_h Single panel height
  // @param rows Number of panels vertically (layout_rows)
  // @param cols Number of panels horizontally (layout_cols)
  // @return Remapped coordinates (physical chain space)
  static constexpr Coords remap(Coords c, Hub75PanelLayout layout, uint16_t panel_w,
                                                       uint16_t panel_h, uint16_t rows, uint16_t cols) {
    // HORIZONTAL: No transformation needed
    // Single row of panels, left-to-right
    // Virtual coordinates map directly to physical chain
    if (layout == Hub75PanelLayout::HORIZONTAL) {
      return c;
    }

    // Common calculations for all grid layouts (row-major)
    // Determine which row of panels (horizontal chain) and local coordinates
    int row = c.y / panel_h;                    // Which horizontal row of panels (0-based, top to bottom)
    int local_y = c.y % panel_h;                // Y within the panel
    int virtual_res_x = cols * panel_w;         // Virtual display width
    int dma_res_x = panel_w * rows * cols - 1;  // Total DMA width - 1 (for mirroring calculations)

    switch (layout) {
      case Hub75PanelLayout::TOP_LEFT_DOWN: {
        // Serpentine: Start top-left, rows chain left→right, then right→left, alternating
        // Even rows (0, 2, ...): reversed X, inverted Y
        // Odd rows (1, 3, ...): normal X, normal Y
        if ((row & 1) == 0) {  // Even rows
          c.x = dma_res_x - c.x - (row * virtual_res_x);
          c.y = panel_h - 1 - local_y;
        } else {  // Odd rows
          c.x = ((rows - (row + 1)) * virtual_res_x) + c.x;
          c.y = local_y;
        }
        break;
      }

      case Hub75PanelLayout::TOP_RIGHT_DOWN: {
        // Serpentine: Start top-right, rows chain right→left, then left→right, alternating
        // Odd rows (1, 3, ...): reversed X, inverted Y
        // Even rows (0, 2, ...): normal X, normal Y
        if ((row & 1) == 1) {  // Odd rows
          c.x = dma_res_x - c.x - (row * virtual_res_x);
          c.y = panel_h - 1 - local_y;
        } else {  // Even rows
          c.x = ((rows - (row + 1)) * virtual_res_x) + c.x;
          c.y = local_y;
        }
        break;
      }

      case Hub75PanelLayout::BOTTOM_LEFT_UP: {
        // Serpentine: Start bottom-left, rows chain left→right, then right→left, alternating (bottom to top)
        // Row numbering inverted: physical bottom row = row 0 in chain
        int inverted_row = rows - 1 - row;
        if ((inverted_row & 1) == 1) {  // Odd inverted rows
          c.x = ((rows - (inverted_row + 1)) * virtual_res_x) + c.x;
          c.y = local_y;
        } else {  // Even inverted rows
          c.x = dma_res_x - (inverted_row * virtual_res_x) - c.x;
          c.y = panel_h - 1 - local_y;
        }
        break;
      }

      case Hub75PanelLayout::BOTTOM_RIGHT_UP: {
        // Serpentine: Start bottom-right, rows chain right→left, then left→right (bottom to top)
        int inverted_row = rows - 1 - row;
        if ((inverted_row & 1) == 0) {  // Even inverted rows
          c.x = ((rows - (inverted_row + 1)) * virtual_res_x) + c.x;
          c.y = local_y;
        } else {  // Odd inverted rows
          c.x = dma_res_x - (inverted_row * virtual_res_x) - c.x;
          c.y = panel_h - 1 - local_y;
        }
        break;
      }

      case Hub75PanelLayout::TOP_LEFT_DOWN_ZIGZAG: {
        // Zigzag: Start top-left, all panels upright (no Y-inversion)
        // All rows use the same formula (no alternating)
        c.x = ((rows - (row + 1)) * virtual_res_x) + c.x;
        c.y = local_y;
        break;
      }

      case Hub75PanelLayout::TOP_RIGHT_DOWN_ZIGZAG: {
        // Zigzag: Start top-right, all panels upright (no Y-inversion)
        // All rows use the same formula (no alternating)
        c.x = ((rows - (row + 1)) * virtual_res_x) + c.x;
        c.y = local_y;
        break;
      }

      case Hub75PanelLayout::BOTTOM_LEFT_UP_ZIGZAG: {
        // Zigzag: Start bottom-left, all panels upright (no Y-inversion)
        // All rows use the same formula (no alternating)
        int inverted_row = rows - 1 - row;
        c.x = ((rows - (inverted_row + 1)) * virtual_res_x) + c.x;
        c.y = local_y;
        break;
      }

      case Hub75PanelLayout::BOTTOM_RIGHT_UP_ZIGZAG: {
        // Zigzag: Start bottom-right, all panels upright (no Y-inversion)
        // All rows use the same formula (no alternating)
        int inverted_row = rows - 1 - row;
        c.x = ((rows - (inverted_row + 1)) * virtual_res_x) + c.x;
        c.y = local_y;
        break;
      }

      default:
        // Fallback: no transformation
        return c;
    }

    // Return remapped coordinates
    // X coordinate has been transformed to physical DMA position
    // Y coordinate is either normal (zigzag) or inverted (serpentine)
    return c;
  }
};


}  // namespace esphome::hub75_icn2053
