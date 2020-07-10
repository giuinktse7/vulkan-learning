#pragma once

#include <unordered_map>
#include <memory>
#include <filesystem>
#include <unordered_map>

#include "protobuf/appearances.pb.h"
#include "../const.h"

struct CatalogInfo
{
  std::filesystem::path file;
  SpriteLayout spriteType;
  uint32_t firstSpriteId;
  uint32_t lastSpriteId;
  uint8_t area;
};

class Appearances
{
  using AppearanceId = uint32_t;

public:
  static void loadFromFile(const std::filesystem::path path);
  static void loadCatalog(const std::filesystem::path path);
  static void loadTextureAtlases();

  //static std::vector<std::vector<uint8_t>> getSprites(uint16_t serverId);
  //static std::vector<std::vector<uint8_t>> getSprites(Item &item);

  static bool contains(AppearanceId id)
  {
    return appearances.find(id) != appearances.end();
  }

  static tibia::protobuf::appearances::Appearance &getById(AppearanceId id)
  {
    return appearances.at(id);
  }

  static CatalogInfo getCatalogInfo(uint32_t spriteId);

  // Maps the upper sprite id bound to info about the sprite sheet
  static std::unordered_map<uint32_t, CatalogInfo> catalogInfo;

private:
  static void addSpriteSheetInfo(CatalogInfo &info);

  static std::unordered_map<AppearanceId, tibia::protobuf::appearances::Appearance> appearances;

  // Catalog content stuff

  /* 
		Used for quick retrieval of the correct spritesheet given a sprite ID.
		It stores the upper bound of the sprite ids in the sprite sheet.
	*/
  static std::set<uint32_t> catalogIndex;
};