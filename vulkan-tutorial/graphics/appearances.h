#pragma once

#include <unordered_map>
#include <memory>
#include <filesystem>
#include <unordered_map>

#ifndef MESSAGES_WRAPPER_H
#define MESSAGES_WRAPPER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251 4100 4267)
#endif

#include "protobuf/appearances.pb.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

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

  static bool hasObject(AppearanceId id)
  {
    return objects.find(id) != objects.end();
  }

  static tibia::protobuf::appearances::Appearance &getObjectById(AppearanceId id)
  {
    return objects.at(id);
  }

  static CatalogInfo getCatalogInfo(uint32_t spriteId);

  // Maps the upper sprite id bound to info about the sprite sheet
  static std::unordered_map<uint32_t, CatalogInfo> catalogInfo;

  static bool isLoaded;

private:
  static void addSpriteSheetInfo(CatalogInfo &info);

  static std::unordered_map<AppearanceId, tibia::protobuf::appearances::Appearance> objects;
  static std::unordered_map<AppearanceId, tibia::protobuf::appearances::Appearance> outfits;

  // Catalog content stuff

  /* 
		Used for quick retrieval of the correct spritesheet given a sprite ID.
		It stores the upper bound of the sprite ids in the sprite sheet.
	*/
  static std::set<uint32_t> catalogIndex;
};