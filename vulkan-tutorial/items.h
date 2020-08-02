#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <tl/expected.hpp>
#include <memory>
#include <pugixml.hpp>
#include <vector>
#include <set>

#include "otb.h"
#include "const.h"

#include "position.h"
#include "graphics/texture_atlas.h"
#include "graphics/protobuf/appearances.pb.h"

class Appearances;

enum FloorChange
{
  Down,
  West,
  East,
  North,
  South,
  WestEx,
  EastEx,
  NorthEx,
  SouthEx,
  SouthAlt,
  EastAlt,
  None
};

enum SlotPositionBits : uint32_t
{
  SLOTP_WHEREEVER = 0xFFFFFFFF,
  SLOTP_HEAD = 1 << 0,
  SLOTP_NECKLACE = 1 << 1,
  SLOTP_BACKPACK = 1 << 2,
  SLOTP_ARMOR = 1 << 3,
  SLOTP_RIGHT = 1 << 4,
  SLOTP_LEFT = 1 << 5,
  SLOTP_LEGS = 1 << 6,
  SLOTP_FEET = 1 << 7,
  SLOTP_RING = 1 << 8,
  SLOTP_AMMO = 1 << 9,
  SLOTP_DEPOT = 1 << 10,
  SLOTP_TWO_HAND = 1 << 11,
  SLOTP_HAND = (SLOTP_LEFT | SLOTP_RIGHT)
};

enum itemgroup_t : uint8_t
{
  ITEM_GROUP_NONE,

  ITEM_GROUP_GROUND,
  ITEM_GROUP_CONTAINER,
  ITEM_GROUP_WEAPON,     //deprecated
  ITEM_GROUP_AMMUNITION, //deprecated
  ITEM_GROUP_ARMOR,      //deprecated
  ITEM_GROUP_CHARGES,
  ITEM_GROUP_TELEPORT,   //deprecated
  ITEM_GROUP_MAGICFIELD, //deprecated
  ITEM_GROUP_WRITEABLE,  //deprecated
  ITEM_GROUP_KEY,        //deprecated
  ITEM_GROUP_SPLASH,
  ITEM_GROUP_FLUID,
  ITEM_GROUP_DOOR, //deprecated
  ITEM_GROUP_DEPRECATED,

  ITEM_GROUP_LAST
};

enum itemattribute_t : uint8_t
{
  ITEM_ATTR_FIRST = 0x10,
  ITEM_ATTR_SERVERID = ITEM_ATTR_FIRST,
  ITEM_ATTR_CLIENTID,
  ITEM_ATTR_NAME,
  ITEM_ATTR_DESCR,
  ITEM_ATTR_SPEED,
  ITEM_ATTR_SLOT,
  ITEM_ATTR_MAXITEMS,
  ITEM_ATTR_WEIGHT,
  ITEM_ATTR_WEAPON,
  ITEM_ATTR_AMU,
  ITEM_ATTR_ARMOR,
  ITEM_ATTR_MAGLEVEL,
  ITEM_ATTR_MAGFIELDTYPE,
  ITEM_ATTR_WRITEABLE,
  ITEM_ATTR_ROTATETO,
  ITEM_ATTR_DECAY,
  ITEM_ATTR_SPRITEHASH,
  ITEM_ATTR_MINIMAPCOLOR,
  ITEM_ATTR_07,
  ITEM_ATTR_08,
  ITEM_ATTR_LIGHT,

  //1-byte aligned
  ITEM_ATTR_DECAY2,     //deprecated
  ITEM_ATTR_WEAPON2,    //deprecated
  ITEM_ATTR_AMU2,       //deprecated
  ITEM_ATTR_ARMOR2,     //deprecated
  ITEM_ATTR_WRITEABLE2, //deprecated
  ITEM_ATTR_LIGHT2,
  ITEM_ATTR_TOPORDER,
  ITEM_ATTR_WRITEABLE3, //deprecated

  ITEM_ATTR_WAREID,

  ITEM_ATTR_LAST
};

enum ItemTypes_t : uint8_t
{
  ITEM_TYPE_NONE,
  ITEM_TYPE_DEPOT,
  ITEM_TYPE_MAILBOX,
  ITEM_TYPE_TRASHHOLDER,
  ITEM_TYPE_CONTAINER,
  ITEM_TYPE_DOOR,
  ITEM_TYPE_MAGICFIELD,
  ITEM_TYPE_TELEPORT,
  ITEM_TYPE_BED,
  ITEM_TYPE_KEY,
  ITEM_TYPE_RUNE,
  ITEM_TYPE_LAST
};

enum ItemFlags_t
{
  FLAG_BLOCK_SOLID = 1 << 0,
  FLAG_BLOCK_PROJECTILE = 1 << 1,
  FLAG_BLOCK_PATHFIND = 1 << 2,
  FLAG_HAS_HEIGHT = 1 << 3,
  FLAG_USEABLE = 1 << 4,
  FLAG_PICKUPABLE = 1 << 5,
  FLAG_MOVEABLE = 1 << 6,
  FLAG_STACKABLE = 1 << 7,
  FLAG_FLOORCHANGEDOWN = 1 << 8,   // unused
  FLAG_FLOORCHANGENORTH = 1 << 9,  // unused
  FLAG_FLOORCHANGEEAST = 1 << 10,  // unused
  FLAG_FLOORCHANGESOUTH = 1 << 11, // unused
  FLAG_FLOORCHANGEWEST = 1 << 12,  // unused
  FLAG_ALWAYSONTOP = 1 << 13,
  FLAG_READABLE = 1 << 14,
  FLAG_ROTATABLE = 1 << 15,
  FLAG_HANGABLE = 1 << 16,
  FLAG_VERTICAL = 1 << 17,
  FLAG_HORIZONTAL = 1 << 18,
  FLAG_CANNOTDECAY = 1 << 19, // unused
  FLAG_ALLOWDISTREAD = 1 << 20,
  FLAG_UNUSED = 1 << 21,        // unused
  FLAG_CLIENTCHARGES = 1 << 22, /* deprecated */
  FLAG_LOOKTHROUGH = 1 << 23,
  FLAG_ANIMATION = 1 << 24,
  FLAG_FULLTILE = 1 << 25, // unused
  FLAG_FORCEUSE = 1 << 26,
};

class ItemType
{
public:
  //non-copyable
  ItemType(const ItemType &other) = delete;
  ItemType &operator=(const ItemType &other) = delete;

  ItemType(ItemType &&) = default;
  ItemType &operator=(ItemType &&) = default;

  ItemType() {}
  ~ItemType();

  void loadTextureAtlas();
  const TextureWindow getTextureWindow() const;

  std::vector<CatalogInfo> catalogInfos() const;

  uint32_t textureAtlasOffset()
  {
    uint32_t spriteId = this->appearance->frame_group().at(0).sprite_info().sprite_id(0);
    return spriteId - getTextureAtlas()->firstSpriteId;
  }

  TextureAtlas *getTextureAtlas();

  bool isGroundTile() const
  {
    return group == ITEM_GROUP_GROUND;
  }
  bool isContainer() const
  {
    return group == ITEM_GROUP_CONTAINER;
  }
  bool isSplash() const
  {
    return group == ITEM_GROUP_SPLASH;
  }
  bool isFluidContainer() const
  {
    return group == ITEM_GROUP_FLUID;
  }

  bool isDoor() const
  {
    return (type == ITEM_TYPE_DOOR);
  }
  bool isMagicField() const
  {
    return (type == ITEM_TYPE_MAGICFIELD);
  }
  bool isTeleport() const
  {
    return (type == ITEM_TYPE_TELEPORT);
  }
  bool isKey() const
  {
    return (type == ITEM_TYPE_KEY);
  }
  bool isDepot() const
  {
    return (type == ITEM_TYPE_DEPOT);
  }
  bool isMailbox() const
  {
    return (type == ITEM_TYPE_MAILBOX);
  }
  bool isTrashHolder() const
  {
    return (type == ITEM_TYPE_TRASHHOLDER);
  }
  bool isBed() const
  {
    return (type == ITEM_TYPE_BED);
  }

  bool isRune() const
  {
    return (type == ITEM_TYPE_RUNE);
  }
  bool isPickupable() const
  {
    return (allowPickupable || pickupable);
  }
  bool isUseable() const
  {
    return (useable);
  }
  bool hasSubType() const
  {
    return (isFluidContainer() || isSplash() || stackable || charges != 0);
  }

  bool usesSubType() const
  {
    return isStackable() || isSplash() || isFluidContainer();
  }

  bool isStackable() const { return stackable; }

  /*
    The items.otb may report a client ID that is incorrect. One such example
    is server ID 2812 that according to the items.otb has client ID 395.
    But there is no object with client ID 395.
    If the client ID is incorrect, it is set to 0.
  */
  bool isValid() const
  {
    return clientId != 0;
  }

  // Abilities &getAbilities()
  // {
  //   if (!abilities)
  //   {
  //     abilities.reset(new Abilities());
  //   }
  //   return *abilities;
  // }

  std::string getPluralName() const
  {
    if (!pluralName.empty())
    {
      return pluralName;
    }

    if (showCount == 0)
    {
      return name;
    }

    std::string str;
    str.reserve(name.length() + 1);
    str.assign(name);
    str += 's';
    return str;
  }

  itemgroup_t group = ITEM_GROUP_NONE;
  ItemTypes_t type = ITEM_TYPE_NONE;
  uint16_t id = 0;
  uint16_t clientId = 0;
  bool stackable = false;
  bool isAnimation = false;

  std::string editorsuffix;

  std::string name;
  std::string article;
  std::string pluralName;
  std::string description;
  std::string runeSpellName;
  std::string vocationString;

  // std::unique_ptr<Abilities> abilities;
  // std::unique_ptr<ConditionDamage> conditionDamage;

  bool decays = false;

  uint32_t weight = 0;
  uint32_t levelDoor = 0;
  uint32_t decayTime = 0;
  uint32_t wieldInfo = 0;
  uint32_t minReqLevel = 0;
  uint32_t minReqMagicLevel = 0;
  uint32_t charges = 0;
  int32_t maxHitChance = -1;
  int32_t decayTo = -1;
  int32_t attack = 0;
  int32_t defense = 0;
  int32_t extraDefense = 0;
  int32_t armor = 0;
  uint16_t rotateTo = 0;
  int32_t runeMagLevel = 0;
  int32_t runeLevel = 0;

  // CombatType_t combatType = COMBAT_NONE;
  uint16_t volume;

  uint16_t transformToOnUse[2] = {0, 0};
  uint16_t transformToFree = 0;
  uint16_t destroyTo = 0;
  uint16_t maxTextLen = 0;
  uint16_t writeOnceItemId = 0;
  uint16_t transformEquipTo = 0;
  uint16_t transformDeEquipTo = 0;
  uint16_t maxItems = 8;
  uint16_t slotPosition = SLOTP_HAND;
  uint16_t speed = 0;
  uint16_t wareId = 0;

  MagicEffectClasses magicEffect = CONST_ME_NONE;
  Direction bedPartnerDir = DIRECTION_NONE;
  WeaponType_t weaponType = WEAPON_NONE;
  Ammo_t ammoType = AMMO_NONE;
  ShootType_t shootType = CONST_ANI_NONE;
  RaceType_t corpseType = RACE_NONE;
  FluidTypes_t fluidSource = FLUID_NONE;

  FloorChange floorChange = FloorChange::None;
  uint8_t alwaysOnTopOrder = 0;
  uint8_t lightLevel = 0;
  uint8_t lightColor = 0;
  uint8_t shootRange = 1;
  int8_t hitChance = 0;

  bool forceUse = false;
  bool forceSerialize = false;
  bool hasHeight = false;
  bool walkStack = true;
  bool blockSolid = false;
  bool blockPickupable = false;
  bool blockProjectile = false;
  bool blockPathFind = false;
  bool allowPickupable = false;
  bool showDuration = false;
  bool showCharges = false;
  bool showAttributes = false;
  bool replaceable = true;
  bool pickupable = false;
  bool rotatable = false;
  bool useable = false;
  bool moveable = false;
  bool alwaysOnTop = false;
  bool canReadText = false;
  bool canWriteText = false;
  bool isVertical = false;
  bool isHorizontal = false;
  bool isHangable = false;
  bool allowDistRead = false;
  bool lookThrough = false;
  bool stopTime = false;
  bool showCount = true;

  TextureAtlas *textureAtlas = nullptr;
  tibia::protobuf::appearances::Appearance *appearance = nullptr;

private:
  // This item's index in the itemtype vector
  uint16_t internalMapId = 0;
};

class Items
{
public:
  static tl::expected<void, std::string> loadFromOtb(const std::string &file);

  static tl::expected<void, std::string> loadFromXml(const std::filesystem::path path);

  bool reload();
  void clear();

  const bool hasItemType(size_t id) const
  {
    return itemTypes.size() > id;
  }

  ItemType *getItemType(uint16_t id);
  ItemType *getItemTypeByClientId(uint16_t clientId);

  const ItemType &getItemIdByClientId(uint16_t spriteId) const;

  uint16_t getItemIdByName(const std::string &name);

  size_t size() const
  {
    return itemTypes.size();
  }

  static Items items;

  ItemType *getNextValidItemType(uint16_t serverId);
  ItemType *getPreviousValidItemType(uint16_t serverId);

  OTB::VersionInfo getOtbVersionInfo();

private:
  Items();
  using ServerID = uint16_t;
  using ClientID = uint16_t;
  // MapID is used to filter out ItemTypes that do not have an appearance (e.g. invalid ItemTypes).
  // using MapID = uint16_t;

  static tl::expected<void, std::string> loadItemFromXml(pugi::xml_node itemNode, uint32_t id);

  std::vector<ItemType> itemTypes;
  std::unordered_map<ClientID, ServerID> clientIdToServerId;
  // std::unordered_map<ServerID, MapID> serverIdToMapId;
  std::unordered_multimap<std::string, ServerID> nameToItems;

  OTB::VersionInfo otbVersionInfo;

  /* Used to hide invalid item ids */
  // TODO Neither are used right now
  std::set<uint16_t> validItemTypeStartId;
  std::set<uint16_t> validItemTypeEndId;
};