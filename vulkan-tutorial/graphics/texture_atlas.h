#pragma once

#include <filesystem>
#include <memory>
#include <variant>

#include "compression.h"
#include "texture.h"
#include "appearances.h"

struct TextureAtlas
{
	using CompressedBytes = std::vector<uint8_t>;
	TextureAtlas(uint32_t id, CompressedBytes &buffer, uint32_t width, uint32_t height, uint32_t firstSpriteId, SpriteLayout spriteLayout, std::filesystem::path sourceFile);
	static std::unique_ptr<TextureAtlas> fromCatalogInfo(CatalogInfo catalogInfo);

	std::filesystem::path sourceFile;

	uint32_t firstSpriteId;
	uint32_t lastSpriteId;

	uint32_t width;
	uint32_t height;

	uint32_t id;

	uint32_t rows;
	uint32_t columns;

	uint32_t spriteWidth;
	uint32_t spriteHeight;

	const TextureWindow getTextureWindow(uint32_t offset) const;

	Texture &getTexture();

	VkDescriptorSet getDescriptorSet()
	{
		return getTexture().getDescriptorSet();
	}

private:
	std::variant<CompressedBytes, Texture> texture;

	void validateBmp(std::vector<uint8_t> buffer);
};
