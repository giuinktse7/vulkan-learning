#include "texture_atlas.h"

#include "../file.h"
#include "compression.h"

#include "../logger.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <iostream>

#include "../debug.h"

constexpr uint32_t SPRITE_SIZE = 32;

// Signifies that the BMP is uncompressed.
constexpr uint8_t BI_BITFIELDS = 0x03;

// See https://en.wikipedia.org/wiki/BMP_file_format#Bitmap_file_header
constexpr uint32_t OFFSET_OF_BMP_START_OFFSET = 10;

TextureAtlas::TextureAtlas(uint32_t id, CompressedBytes &buffer, uint32_t width, uint32_t height, uint32_t firstSpriteId, SpriteLayout spriteLayout, std::filesystem::path sourceFile)
    : texture(buffer), id(id), width(width), height(height), firstSpriteId(firstSpriteId), lastSpriteId(id), sourceFile(sourceFile)
{
  switch (spriteLayout)
  {
  case SpriteLayout::ONE_BY_ONE:
    this->rows = 12;
    this->columns = 12;
    this->spriteWidth = SPRITE_SIZE;
    this->spriteHeight = SPRITE_SIZE;
    drawOffset.x = 0;
    drawOffset.y = 0;
    break;
  case SpriteLayout::ONE_BY_TWO:
    this->rows = 6;
    this->columns = 12;
    this->spriteWidth = SPRITE_SIZE;
    this->spriteHeight = SPRITE_SIZE * 2;
    drawOffset.x = 0;
    drawOffset.y = -1;
    break;
  case SpriteLayout::TWO_BY_ONE:
    this->rows = 12;
    this->columns = 6;
    this->spriteWidth = SPRITE_SIZE * 2;
    this->spriteHeight = SPRITE_SIZE;
    drawOffset.x = -1;
    drawOffset.y = 0;
    break;
  case SpriteLayout::TWO_BY_TWO:
    this->rows = 6;
    this->columns = 6;
    this->spriteWidth = SPRITE_SIZE * 2;
    this->spriteHeight = SPRITE_SIZE * 2;
    drawOffset.x = -1;
    drawOffset.y = -1;
    break;
  default:
    this->rows = 12;
    this->columns = 12;
    this->spriteWidth = SPRITE_SIZE;
    this->spriteHeight = SPRITE_SIZE;
    drawOffset.x = 0;
    drawOffset.y = 0;
    break;
  }

  assert(id == lastSpriteId);
}

const TextureWindow TextureAtlas::getTextureWindow(uint32_t spriteId) const
{
  DEBUG_ASSERT(firstSpriteId <= spriteId && spriteId <= lastSpriteId, "The TextureAtlas does not contain that sprite ID.");

  uint32_t offset = spriteId - this->firstSpriteId;

  auto row = offset / columns;
  auto col = offset % columns;

  const float x = static_cast<float>(col) / columns;
  const float y = static_cast<float>(rows - row) / rows;

  const float width = static_cast<float>(spriteWidth) / this->width;
  const float height = static_cast<float>(spriteHeight) / this->height;

  return TextureWindow{x, y, x + width, y - height};
}

void print_byte(uint8_t b)
{
  std::cout << std::hex << std::setw(2) << unsigned(b) << std::endl;
}

void nextN(std::vector<uint8_t> &buffer, uint32_t offset, uint32_t n)
{
  //std::cout << "Next " << std::to_string(n) << ":" << std::endl;
  uint8_t k = 0;
  uint8_t *data = buffer.data();
  for (uint32_t i = 0; i < n; ++i)
  {
    print_byte(*(data + offset + i));
  }

  std::cout << "---" << std::endl;
}

uint32_t readU32(std::vector<uint8_t> &buffer, uint32_t offset)
{
  uint32_t value;
  std::memcpy(&value, buffer.data() + offset, sizeof(uint32_t));
  return value;
}

void TextureAtlas::validateBmp(std::vector<uint8_t> decompressed)
{
  uint32_t width = readU32(decompressed, 0x12);
  if (width != TextureAtlasSize.width)
  {
    throw std::runtime_error("Texture atlas has incorrect width. Expected " + std::to_string(TextureAtlasSize.width) + " but received " + std::to_string(width) + ".");
  }

  uint32_t height = readU32(decompressed, 0x16);
  if (height != TextureAtlasSize.height)
  {
    throw std::runtime_error("Texture atlas has incorrect width. Expected " + std::to_string(TextureAtlasSize.height) + " but received " + std::to_string(height) + ".");
  }

  uint32_t compression = readU32(decompressed, 0x1E);
  if (compression != BI_BITFIELDS)
  {
    throw std::runtime_error("Texture atlas has incorrect compression. Expected BI_BITFIELDS but received " + std::to_string(compression) + ".");
  }
}

void fixMagenta(std::vector<uint8_t> &buffer, uint32_t offset)
{
  for (auto cursor = 0; cursor < buffer.size() - 4 - offset; cursor += 4)
  {
    if (readU32(buffer, offset + cursor) == 0xFF00FF)
    {
      std::fill(buffer.begin() + offset + cursor, buffer.begin() + offset + cursor + 3, 0);
    }
  }
}

Texture &TextureAtlas::getTexture()
{
  if (std::holds_alternative<CompressedBytes>(texture))
  {
    Logger::debug("Decompressed " + this->sourceFile.string());

    std::vector<uint8_t> decompressed = LZMA::decompress(std::get<CompressedBytes>(this->texture));

    validateBmp(decompressed);

    uint32_t offset;
    std::memcpy(&offset, decompressed.data() + OFFSET_OF_BMP_START_OFFSET, sizeof(uint32_t));

    fixMagenta(decompressed, offset);

    // nextN(decompressed, offset, 4);
    // std::cout << unsigned(readU32(decompressed, offset)) << std::endl;

    texture = Texture(this->width, this->height, std::vector<uint8_t>(decompressed.begin() + offset, decompressed.end()));
  }

  return std::get<Texture>(this->texture);
}