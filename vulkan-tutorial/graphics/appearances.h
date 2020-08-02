#pragma once

#include <unordered_map>
#include <memory>
#include <filesystem>
#include <unordered_map>

#include <sstream>
#include <variant>

#ifndef MESSAGES_WRAPPER_H
#define MESSAGES_WRAPPER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251 4100 4267)
#endif

#include "protobuf/appearances.pb.h"

#include "../debug.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

#include "../position.h"
#include "../const.h"

enum class AppearanceFlags : uint64_t
{
  Bank = 1ULL << 0,
  Clip = 1ULL << 1,
  Bottom = 1ULL << 2,
  Top = 1ULL << 3,
  Container = 1ULL << 4,
  Cumulative = 1ULL << 5,
  Usable = 1ULL << 6,
  Forceuse = 1ULL << 7,
  Multiuse = 1ULL << 8,
  Write = 1ULL << 9,
  WriteOnce = 1ULL << 10,
  Liquidpool = 1ULL << 11,
  Unpass = 1ULL << 12,
  Unmove = 1ULL << 13,
  Unsight = 1ULL << 14,
  Avoid = 1ULL << 15,
  NoMovementAnimation = 1ULL << 16,
  Take = 1ULL << 17,
  LiquidContainer = 1ULL << 18,
  Hang = 1ULL << 19,
  Hook = 1ULL << 20,
  Rotate = 1ULL << 21,
  Light = 1ULL << 22,
  DontHide = 1ULL << 23,
  Translucent = 1ULL << 24,
  Shift = 1ULL << 25,
  Height = 1ULL << 26,
  LyingObject = 1ULL << 27,
  AnimateAlways = 1ULL << 28,
  Automap = 1ULL << 29,
  Lenshelp = 1ULL << 30,
  Fullbank = 1ULL << 31,
  IgnoreLook = 1ULL << 32,
  Clothes = 1ULL << 33,
  DefaultAction = 1ULL << 34,
  Market = 1ULL << 35,
  Wrap = 1ULL << 36,
  Unwrap = 1ULL << 37,
  Topeffect = 1ULL << 38,
  NpcSaleData = 1ULL << 39,
  ChangedToExpire = 1ULL << 40,
  Corpse = 1ULL << 41,
  PlayerCorpse = 1ULL << 42,
  CyclopediaItem = 1ULL << 43
};

enum class AnimationLoopType
{
  PingPong = -1,
  Infinite = 0,
  Counted = 1
};

inline AppearanceFlags operator|(AppearanceFlags a, AppearanceFlags b)
{
  return static_cast<AppearanceFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline AppearanceFlags operator&(AppearanceFlags a, AppearanceFlags b)
{
  return static_cast<AppearanceFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline AppearanceFlags operator&=(AppearanceFlags a, AppearanceFlags b)
{
  return (AppearanceFlags &)((uint64_t &)a &= (uint64_t)b);
}

inline AppearanceFlags &operator|=(AppearanceFlags &a, AppearanceFlags b)
{
  return (AppearanceFlags &)((uint64_t &)a |= (uint64_t)b);
}

inline bool operator>(AppearanceFlags a, uint64_t b)
{
  return (uint64_t)a > b;
}

struct CatalogInfo
{
  std::filesystem::path file;
  SpriteLayout spriteType;
  uint32_t firstSpriteId;
  uint32_t lastSpriteId;
  uint8_t area;
};

struct SpritePhase
{
  uint32_t minDuration;
  uint32_t maxDuration;
};

enum FixedFrameGroup
{
  OutfitIdle = 0,
  OutfitMoving = 1,
  ObjectInitial = 2
};

struct SpriteAnimation
{
  uint32_t defaultStartPhase;
  bool synchronized;
  bool randomStartPhase;
  AnimationLoopType loopType;

  // Amount of times to loop. Only relevant if the loopType is AnimationLoopType::Counted.
  uint32_t loopCount;
  std::vector<SpritePhase> phases;

  static SpriteAnimation fromProtobufData(tibia::protobuf::appearances::SpriteAnimation animation);
};

struct SpriteInfo
{
  uint32_t patternWidth;
  uint32_t patternHeight;
  uint32_t patternDepth;
  uint32_t layers;

  std::vector<uint32_t> spriteIds;

  uint32_t boundingSquare;
  bool isOpaque;

  SpriteAnimation *getAnimation()
  {
    return animation.get();
  }

  static SpriteInfo fromProtobufData(tibia::protobuf::appearances::SpriteInfo info);

  // repeated Box bounding_box_per_direction = 9;
private:
  std::unique_ptr<SpriteAnimation> animation;
};

struct FrameGroup
{
  FixedFrameGroup fixedGroup;
  uint32_t id;
  SpriteInfo spriteInfo;
};

/*
  Convenience wrapper for a tibia::protobuf Appearance
*/
class Appearance
{
public:
  Appearance(tibia::protobuf::appearances::Appearance protobufAppearance);
  Appearance(const Appearance &) = delete;

  uint32_t getSpriteId(uint32_t frameGroup, Position pos);
  uint32_t getFirstSpriteId() const;
  const SpriteInfo &getSpriteInfo(size_t frameGroup) const;
  const SpriteInfo &getSpriteInfo() const;

  bool hasFlag(AppearanceFlags flag)
  {
    return (flags & flag) > 0ULL;
  }

  size_t frameGroupCount() const;

  uint32_t clientId;
  std::string name;

private:
  // Most (if not all) objects have only one FrameGroup. This avoids having to create a vector to store a single element (perf not tested as of 2020/08/02).
  std::variant<SpriteInfo, std::vector<FrameGroup>> appearanceData;
  AppearanceFlags flags;
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

  static Appearance &getObjectById(AppearanceId id)
  {
    return objects.at(id);
  }

  static CatalogInfo getCatalogInfo(uint32_t spriteId);

  // Maps the upper sprite id bound to info about the sprite sheet
  static std::unordered_map<uint32_t, CatalogInfo> catalogInfo;

  static bool isLoaded;

private:
  static void addSpriteSheetInfo(CatalogInfo &info);

  static std::unordered_map<AppearanceId, Appearance> objects;
  static std::unordered_map<AppearanceId, tibia::protobuf::appearances::Appearance> outfits;

  // Catalog content stuff

  /* 
		Used for quick retrieval of the correct spritesheet given a sprite ID.
		It stores the upper bound of the sprite ids in the sprite sheet.
	*/
  static std::set<uint32_t> catalogIndex;
};

inline uint32_t Appearance::getSpriteId(uint32_t frameGroup, Position pos)
{
  DEBUG_ASSERT(static_cast<size_t>(frameGroup) < frameGroupCount(), "Bad framegroup index: " + std::to_string(frameGroup) + ", there are only " + std::to_string(frameGroupCount()) + " frame groups.");
  const SpriteInfo &spriteInfo = getSpriteInfo(frameGroup);

  uint32_t x = spriteInfo.patternWidth;
  uint32_t y = spriteInfo.patternHeight;
  uint32_t z = spriteInfo.patternDepth;

  uint32_t index = pos.x % x + pos.y % y;

  return 0;
}

inline std::ostream &operator<<(std::ostream &os, const tibia::protobuf::appearances::SpriteInfo &info)
{
  std::ostringstream s;

  s << "{ ";
  if (info.has_pattern_width())
    s << "pattern_width=" << info.pattern_width() << ", ";

  if (info.has_pattern_height())
    s << "pattern_height=" << info.pattern_height() << ", ";

  if (info.pattern_depth())
    s << "pattern_depth=" << info.pattern_depth() << ", ";

  if (info.has_layers())
    s << "layers=" << info.layers() << ", ";

  s << "sprite_ids=";

  for (const auto id : info.sprite_id())
    s << id << ", ";

  if (info.has_bounding_square())
    s << "bounding_square=" << info.bounding_square() << ", ";

  if (info.has_animation())
    s << "has_animation= true, ";

  if (info.has_is_opaque())
    s << "is_opaque = " << info.is_opaque() << ", ";

  // Print bounding box information
  s << "bboxes: " << info.bounding_box_per_direction_size() << std::endl;
  // for (const auto bbox : info.bounding_box_per_direction())
  //   s << std::endl
  //     << "\t{ x=" << bbox.x() << ", y=" << bbox.y() << ", width=" << bbox.width() << ", height=" << bbox.height() << "}";

  os << s.str();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const tibia::protobuf::appearances::AppearanceFlags &flags)
{
  std::ostringstream s;
  s << std::endl;

#define PRINT_FLAG_UTIL(a)                                \
  do                                                      \
  {                                                       \
    if (flags.has_##a() && flags.##a())                   \
    {                                                     \
      s << "\t" << #a << ":" << flags.##a() << std::endl; \
    }                                                     \
  } while (false)

  if (flags.has_bank())
  {
    std::cout << "bank.waypoints: " << flags.bank().waypoints() << std::endl;
  }
  PRINT_FLAG_UTIL(clip);
  PRINT_FLAG_UTIL(bottom);
  PRINT_FLAG_UTIL(top);
  PRINT_FLAG_UTIL(container);
  PRINT_FLAG_UTIL(cumulative);
  PRINT_FLAG_UTIL(usable);
  PRINT_FLAG_UTIL(forceuse);
  PRINT_FLAG_UTIL(multiuse);
  PRINT_FLAG_UTIL(liquidpool);
  PRINT_FLAG_UTIL(unpass);
  PRINT_FLAG_UTIL(unmove);
  PRINT_FLAG_UTIL(unsight);
  PRINT_FLAG_UTIL(avoid);
  PRINT_FLAG_UTIL(no_movement_animation);
  PRINT_FLAG_UTIL(take);
  PRINT_FLAG_UTIL(liquidcontainer);
  PRINT_FLAG_UTIL(hang);
  if (flags.has_hook())
  {
    if (flags.hook().has_direction())
      s << "\thook: " << flags.hook().direction() << std::endl;
    else
      s << "\thook (no direction)" << std::endl;
  }

  PRINT_FLAG_UTIL(rotate);
  if (flags.has_light())
  {
    s << "\tlight: ("
      << "brightness: " << flags.light().brightness() << ", color: " << flags.light().color() << ")" << std::endl;
  }
  PRINT_FLAG_UTIL(dont_hide);
  PRINT_FLAG_UTIL(translucent);
  if (flags.has_shift())
  {
    s << "\tshift: (";
    if (flags.shift().has_x())
    {
      s << "x=" << flags.shift().x() << " ";
    }
    if (flags.shift().has_y())
    {
      s << "y=" << flags.shift().y();
    }

    s << ")" << std::endl;
  }

  if (flags.has_height())
  {
    if (flags.height().has_elevation())
    {
      s << "\theight elevation: " << flags.height().elevation() << std::endl;
    }
    else
    {
      s << "\theight (no elevation)" << std::endl;
    }
  }
  PRINT_FLAG_UTIL(lying_object);
  PRINT_FLAG_UTIL(animate_always);
  PRINT_FLAG_UTIL(fullbank);
  PRINT_FLAG_UTIL(ignore_look);
  PRINT_FLAG_UTIL(wrap);
  PRINT_FLAG_UTIL(unwrap);
  PRINT_FLAG_UTIL(topeffect);

  // Print NPC sale data for RL Tibia
  // for (const auto saleData : flags.npcsaledata())
  // {
  //   s << "\t" << saleData.name() << " (" << saleData.location() << "): sells_for=" << saleData.sale_price() << ", buys_for=" << saleData.buy_price() << ", currency=" << saleData.currency() << std::endl;
  // }

  PRINT_FLAG_UTIL(corpse);
  PRINT_FLAG_UTIL(player_corpse);

#undef PRINT_FLAG_UTIL

  os << s.str();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const tibia::protobuf::appearances::SpriteAnimation &animation)
{
  std::ostringstream s;

  s << "default_start_phase: " << animation.default_start_phase() << ", ";
  s << "synchronized: " << animation.synchronized() << ", ";
  s << "random_start_phase: " << animation.random_start_phase() << ", ";

  s << "loop_type: ";

  switch (animation.loop_type())
  {
  case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_PINGPONG:
    s << "ANIMATION_LOOP_TYPE_PINGPONG";
    break;
  case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_INFINITE:
    s << "ANIMATION_LOOP_TYPE_INFINITE";
    break;
  case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_COUNTED:
  default:
    s << "ANIMATION_LOOP_TYPE_COUNTED";
    break;
  }

  s << std::endl;

  s << "loop_count: " << animation.loop_count() << ", " << std::endl;
  s << "phases:" << std::endl;
  for (int i = 0; i < animation.sprite_phase_size(); ++i)
  {
    auto phase = animation.sprite_phase().at(i);
    s << "\t[" << phase.duration_min() << ", " << phase.duration_max() << "]" << std::endl;
  }

  os << s.str();
  return os;
}