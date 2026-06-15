export module quantize;

import std;

// Returns dominant hex color from ARGB pixels.
export std::string dominantColor(const std::vector<std::uint32_t>& pixels, const std::string& fallback) {
  if (pixels.empty()) return fallback;

  // Bucket each pixel into 5-bit per channel (32 levels), skip near-black/white
  std::unordered_map<std::uint32_t, int> counts;
  for (auto pixel : pixels) {
    std::uint8_t r = (pixel >> 16) & 0xFF;
    std::uint8_t g = (pixel >> 8) & 0xFF;
    std::uint8_t b = pixel & 0xFF;
    std::uint8_t qr = r >> 3, qg = g >> 3, qb = b >> 3;
    if (qr < 2 && qg < 2 && qb < 2) continue;    // skip near-black
    if (qr > 29 && qg > 29 && qb > 29) continue; // skip near-white
    counts[((std::uint32_t)qr << 10) | ((std::uint32_t)qg << 5) | qb]++;
  }

  if (counts.empty()) return fallback;

  auto best = std::ranges::max_element(counts, {}, &std::pair<const std::uint32_t, int>::second);
  std::uint8_t r = ((best->first >> 10) & 0x1F) << 3 | 4;
  std::uint8_t g = ((best->first >> 5) & 0x1F) << 3 | 4;
  std::uint8_t b = (best->first & 0x1F) << 3 | 4;
  return std::format("#{:02x}{:02x}{:02x}", r, g, b);
}
