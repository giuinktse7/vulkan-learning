#include "appearances.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>

#include "../util.h"
#include "../file.h"

#include "texture_atlas.h"
#include "../items.h"
#include "engine.h"

#include <set>

using namespace std;

std::unordered_map<uint32_t, Appearance> Appearances::objects;
std::unordered_map<uint32_t, tibia::protobuf::appearances::Appearance> Appearances::outfits;

std::set<uint32_t> Appearances::catalogIndex;
std::unordered_map<uint32_t, CatalogInfo> Appearances::catalogInfo;

std::set<uint32_t> Appearances::textureAtlasIds;
std::unordered_map<uint32_t, std::unique_ptr<TextureAtlas>> Appearances::textureAtlases;

bool Appearances::isLoaded;

/*
	The width and height of a texture atlas in pixels
*/
constexpr struct
{
    uint16_t width = 384;
    uint16_t height = 384;
} TextureAtlasSize;

void Appearances::loadFromFile(const std::filesystem::path path)
{
    TimeMeasure startTime = TimeMeasure::start();

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    tibia::protobuf::appearances::Appearances parsed;

    {
        std::fstream input(path, std::ios::in | std::ios::binary);
        std::cout << path << std::endl;
        if (!parsed.ParseFromIstream(&input))
        {
            auto absolutePath = filesystem::absolute(filesystem::path(path));
            std::cerr << "Failed to parse appearances file at " << absolutePath << "." << endl;
        }

        google::protobuf::ShutdownProtobufLibrary();
    }

    int total = 0;
    for (int i = 0; i < parsed.object_size(); ++i)
    {
        const tibia::protobuf::appearances::Appearance &object = parsed.object(i);
        auto info = object.frame_group().at(0).sprite_info();
        // if (object.id() == 3031 || object.id() == 103)
        // {
        //     std::cout << "\n(cid: " << object.id() << "): " << std::endl;
        //     std::cout << "info: " << info << std::endl;
        //     if (object.has_flags())
        //     {
        //         std::cout << "flags: " << object.flags() << std::endl;
        //     }
        // }

        // if (object.has_flags() && object.flags().has_cumulative() && object.flags().cumulative() && info.sprite_id_size() > 1 && info.has_animation())
        // {
        //     std::cout << "\n(cid: " << object.id() << "): " << std::endl;
        //     std::cout << "framegroups: " << object.frame_group_size() << std::endl;
        //     std::cout << "info: " << info << std::endl;
        //     if (object.has_flags())
        //     {
        //         std::cout << "flags: " << std::endl;
        //         std::cout << object.flags() << std::endl;
        //     }
        //     if (info.has_animation())
        //     {
        //         std::cout << "Animation:" << std::endl
        //                   << info.animation() << std::endl;
        //     }
        //     ++total;
        // }
        Appearances::objects.emplace(object.id(), object);
    }

    std::cout << "Total: " << total << std::endl;

    for (int i = 0; i < parsed.outfit_size(); ++i)
    {
        const tibia::protobuf::appearances::Appearance &outfit = parsed.outfit(i);
        Appearances::outfits[outfit.id()] = outfit;
    }

    Appearances::isLoaded = true;

    std::cout << "Loaded appearances.dat in " << startTime.elapsedMillis() << " milliseconds." << std::endl;
}

void Appearances::addSpriteSheetInfo(CatalogInfo &info)
{
    Appearances::catalogIndex.insert(info.lastSpriteId);
    Appearances::catalogInfo[info.lastSpriteId] = info;
}

CatalogInfo Appearances::getCatalogInfo(uint32_t spriteId)
{
    auto found = lower_bound(catalogIndex.begin(), catalogIndex.end(), spriteId);
    if (found == catalogIndex.end())
    {
        cout << "Could not find a sprite sheet for sprite ID " << spriteId << "." << endl;
        exit(1);
    }

    uint32_t lastSpriteId = *found;

    return catalogInfo.at(lastSpriteId);
}

void Appearances::loadCatalog(const std::filesystem::path path)
{
    if (!filesystem::exists(path))
    {
        cout << "Could not locate the catalog JSON file. Failed to find file at path: " + filesystem::absolute(path).u8string() << endl;
        exit(1);
    }

    std::ifstream fileStream(path);
    auto json = make_unique<nlohmann::json>();
    fileStream >> (*json);

    std::filesystem::path basepath("C:/Users/giuin/AppData/Local/Tibia11/packages/Tibia/assets");

    std::set<std::string> types;

    for (const auto &x : *json)
    {
        if (x.at("type") == "sprite")
        {
            std::string file = x.at("file");
            std::filesystem::path filePath(file);

            CatalogInfo info{};
            info.area = x.at("area");
            info.file = basepath / filePath;
            info.spriteType = x.at("spritetype");
            info.firstSpriteId = x.at("firstspriteid");
            info.lastSpriteId = x.at("lastspriteid");
            addSpriteSheetInfo(info);
        }
        else
        {
            std::string s = x.at("type");
            types.insert(s);
        }
    }

    std::cout << "Loaded compressed sprites in " << start.elapsedMillis() << " milliseconds." << std::endl;
}

SpriteInfo SpriteInfo::fromProtobufData(tibia::protobuf::appearances::SpriteInfo spriteInfo)
{
    SpriteInfo info{};
    if (spriteInfo.has_animation())
    {
        info.animation = std::make_unique<SpriteAnimation>(SpriteAnimation::fromProtobufData(spriteInfo.animation()));
    }
    info.boundingSquare = spriteInfo.bounding_square();
    info.isOpaque = spriteInfo.bounding_square();
    info.layers = spriteInfo.layers();
    info.patternWidth = spriteInfo.pattern_width();
    info.patternHeight = spriteInfo.pattern_height();
    info.patternDepth = spriteInfo.pattern_depth();

    for (auto id : spriteInfo.sprite_id())
    {
        info.spriteIds.emplace_back<uint32_t &>(id);
    }

    return info;
}

SpriteAnimation SpriteAnimation::fromProtobufData(tibia::protobuf::appearances::SpriteAnimation animation)
{
    SpriteAnimation anim{};
    anim.defaultStartPhase = animation.default_start_phase();
    anim.loopCount = animation.loop_count();

    switch (animation.loop_type())
    {
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_PINGPONG:
        anim.loopType = AnimationLoopType::PingPong;
        break;
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_COUNTED:
        anim.loopType = AnimationLoopType::Counted;
        break;
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_INFINITE:
    default:
        anim.loopType = AnimationLoopType::Infinite;
        break;
    }

    for (auto phase : animation.sprite_phase())
    {
        anim.phases.push_back({phase.duration_min(), phase.duration_max()});
    }

    anim.defaultStartPhase = animation.default_start_phase();
    anim.randomStartPhase = animation.random_start_phase();
    anim.synchronized = animation.synchronized();

    return anim;
}

/*
    Constructs an Appearance from protobuf Appearance data.
*/
Appearance::Appearance(tibia::protobuf::appearances::Appearance protobufAppearance)
{
    this->clientId = protobufAppearance.id();
    this->name = protobufAppearance.name();

    if (protobufAppearance.frame_group_size() == 1)
    {
        this->appearanceData = SpriteInfo::fromProtobufData(protobufAppearance.frame_group().at(0).sprite_info());
    }

    // Flags
    this->flags = static_cast<AppearanceFlag>(0);
    if (protobufAppearance.has_flags())
    {
        auto flags = protobufAppearance.flags();

#define ADD_FLAG_UTIL(flagType, flag) \
    do                                \
    {                                 \
        if (flags.has_##flagType())   \
        {                             \
            this->flags |= flag;      \
        }                             \
    } while (false)

        ADD_FLAG_UTIL(bank, AppearanceFlag::Bank);
        ADD_FLAG_UTIL(clip, AppearanceFlag::Clip);
        ADD_FLAG_UTIL(bottom, AppearanceFlag::Bottom);
        ADD_FLAG_UTIL(top, AppearanceFlag::Top);
        ADD_FLAG_UTIL(container, AppearanceFlag::Container);
        ADD_FLAG_UTIL(cumulative, AppearanceFlag::Cumulative);
        ADD_FLAG_UTIL(usable, AppearanceFlag::Usable);
        ADD_FLAG_UTIL(forceuse, AppearanceFlag::Forceuse);
        ADD_FLAG_UTIL(multiuse, AppearanceFlag::Multiuse);
        ADD_FLAG_UTIL(write, AppearanceFlag::Write);
        ADD_FLAG_UTIL(write_once, AppearanceFlag::WriteOnce);
        ADD_FLAG_UTIL(liquidpool, AppearanceFlag::Liquidpool);
        ADD_FLAG_UTIL(unpass, AppearanceFlag::Unpass);
        ADD_FLAG_UTIL(unmove, AppearanceFlag::Unmove);
        ADD_FLAG_UTIL(unsight, AppearanceFlag::Unsight);
        ADD_FLAG_UTIL(avoid, AppearanceFlag::Avoid);
        ADD_FLAG_UTIL(no_movement_animation, AppearanceFlag::NoMovementAnimation);
        ADD_FLAG_UTIL(take, AppearanceFlag::Take);
        ADD_FLAG_UTIL(liquidcontainer, AppearanceFlag::LiquidContainer);
        ADD_FLAG_UTIL(hang, AppearanceFlag::Hang);
        ADD_FLAG_UTIL(hook, AppearanceFlag::Hook);
        ADD_FLAG_UTIL(rotate, AppearanceFlag::Rotate);
        ADD_FLAG_UTIL(light, AppearanceFlag::Light);
        ADD_FLAG_UTIL(dont_hide, AppearanceFlag::DontHide);
        ADD_FLAG_UTIL(translucent, AppearanceFlag::Translucent);
        ADD_FLAG_UTIL(shift, AppearanceFlag::Shift);
        ADD_FLAG_UTIL(height, AppearanceFlag::Height);
        ADD_FLAG_UTIL(lying_object, AppearanceFlag::LyingObject);
        ADD_FLAG_UTIL(animate_always, AppearanceFlag::AnimateAlways);
        ADD_FLAG_UTIL(automap, AppearanceFlag::Automap);
        ADD_FLAG_UTIL(lenshelp, AppearanceFlag::Lenshelp);
        ADD_FLAG_UTIL(fullbank, AppearanceFlag::Fullbank);
        ADD_FLAG_UTIL(ignore_look, AppearanceFlag::IgnoreLook);
        ADD_FLAG_UTIL(clothes, AppearanceFlag::Clothes);
        ADD_FLAG_UTIL(default_action, AppearanceFlag::DefaultAction);
        ADD_FLAG_UTIL(market, AppearanceFlag::Market);
        ADD_FLAG_UTIL(wrap, AppearanceFlag::Wrap);
        ADD_FLAG_UTIL(unwrap, AppearanceFlag::Unwrap);
        ADD_FLAG_UTIL(topeffect, AppearanceFlag::Topeffect);
        // TODO npcsaledata flag is not handled right now (probably not needed)
        ADD_FLAG_UTIL(changedtoexpire, AppearanceFlag::ChangedToExpire);
        ADD_FLAG_UTIL(corpse, AppearanceFlag::Corpse);
        ADD_FLAG_UTIL(player_corpse, AppearanceFlag::PlayerCorpse);
        ADD_FLAG_UTIL(cyclopediaitem, AppearanceFlag::CyclopediaItem);

#undef ADD_FLAG_UTIL

        if (hasFlag(AppearanceFlag::Bank))
            flagData.bankWaypoints = flags.bank().waypoints();
        if (hasFlag(AppearanceFlag::Write))
            flagData.maxTextLength = flags.write().max_text_length();
        if (hasFlag(AppearanceFlag::WriteOnce))
            flagData.maxTextLengthOnce = flags.write_once().max_text_length_once();
        if (hasFlag(AppearanceFlag::Hook))
        {
            auto direction = flags.hook().direction();
            if (direction == tibia::protobuf::shared::HOOK_TYPE::HOOK_TYPE_SOUTH)
                flagData.hookDirection = HookType::South;
            else
                flagData.hookDirection = HookType::East;
        }
        if (hasFlag(AppearanceFlag::Light))
        {
            flagData.brightness = flags.light().brightness();
            flagData.color = flags.light().color();
        }

        if (hasFlag(AppearanceFlag::Shift))
        {
            if (flags.shift().has_x())
                flagData.shiftX = flags.shift().x();
            if (flags.shift().has_y())
                flagData.shiftY = flags.shift().y();
        }
        if (hasFlag(AppearanceFlag::Height))
            flagData.elevation = flags.height().elevation();
        if (hasFlag(AppearanceFlag::Automap))
            flagData.automapColor = flags.automap().color();
        if (hasFlag(AppearanceFlag::Lenshelp))
            flagData.lenshelp = flags.lenshelp().id();
        if (hasFlag(AppearanceFlag::Clothes))
            flagData.itemSlot = flags.clothes().slot();
        if (hasFlag(AppearanceFlag::DefaultAction))
        {
            switch (flags.default_action().action())
            {

            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_LOOK:
                flagData.defaultAction = AppearancePlayerDefaultAction::Look;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_USE:
                flagData.defaultAction = AppearancePlayerDefaultAction::Use;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_OPEN:
                flagData.defaultAction = AppearancePlayerDefaultAction::Open;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_AUTOWALK_HIGHLIGHT:
                flagData.defaultAction = AppearancePlayerDefaultAction::AutowalkHighlight;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_NONE:
            default:
                flagData.defaultAction = AppearancePlayerDefaultAction::None;
                break;
            }
        }
        if (hasFlag(AppearanceFlag::Market))
        {
#define MAP_MARKET_FLAG(src, dst)                                     \
    if (1)                                                            \
    {                                                                 \
    case tibia::protobuf::shared::ITEM_CATEGORY::ITEM_CATEGORY_##src: \
        flagData.market.category = dst;                               \
        break;                                                        \
    }                                                                 \
    else

            switch (flags.market().category())
            {
                MAP_MARKET_FLAG(ARMORS, AppearanceItemCategory::Armors);
                MAP_MARKET_FLAG(AMULETS, AppearanceItemCategory::Amulets);
                MAP_MARKET_FLAG(BOOTS, AppearanceItemCategory::Boots);
                MAP_MARKET_FLAG(CONTAINERS, AppearanceItemCategory::Containers);
                MAP_MARKET_FLAG(DECORATION, AppearanceItemCategory::Decoration);
                MAP_MARKET_FLAG(FOOD, AppearanceItemCategory::Food);
                MAP_MARKET_FLAG(HELMETS_HATS, AppearanceItemCategory::HelmetsHats);
                MAP_MARKET_FLAG(LEGS, AppearanceItemCategory::Legs);
                MAP_MARKET_FLAG(OTHERS, AppearanceItemCategory::Others);
                MAP_MARKET_FLAG(POTIONS, AppearanceItemCategory::Potions);
                MAP_MARKET_FLAG(RINGS, AppearanceItemCategory::Rings);
                MAP_MARKET_FLAG(RUNES, AppearanceItemCategory::Runes);
                MAP_MARKET_FLAG(SHIELDS, AppearanceItemCategory::Shields);
                MAP_MARKET_FLAG(TOOLS, AppearanceItemCategory::Tools);
                MAP_MARKET_FLAG(VALUABLES, AppearanceItemCategory::Valuables);
                MAP_MARKET_FLAG(AMMUNITION, AppearanceItemCategory::Ammunition);
                MAP_MARKET_FLAG(AXES, AppearanceItemCategory::Axes);
                MAP_MARKET_FLAG(CLUBS, AppearanceItemCategory::Clubs);
                MAP_MARKET_FLAG(DISTANCE_WEAPONS, AppearanceItemCategory::DistanceWeapons);
                MAP_MARKET_FLAG(SWORDS, AppearanceItemCategory::Swords);
                MAP_MARKET_FLAG(WANDS_RODS, AppearanceItemCategory::WandsRods);
                MAP_MARKET_FLAG(PREMIUM_SCROLLS, AppearanceItemCategory::PremiumScrolls);
                MAP_MARKET_FLAG(TIBIA_COINS, AppearanceItemCategory::TibiaCoins);
                MAP_MARKET_FLAG(CREATURE_PRODUCTS, AppearanceItemCategory::CreatureProducts);
            default:
                ABORT_PROGRAM("Unknown appearance flag market category: " + std::to_string(flags.market().category()));
                break;
            }

#undef MAP_MARKET_FLAG
        }
        // We don't need this flag in a map editor
        // if (hasFlag(AppearanceFlag::NpcSaleData))
        // {
        // }
        if (hasFlag(AppearanceFlag::ChangedToExpire))
            flagData.changedToExpireFormerObjectTypeId = flags.changedtoexpire().former_object_typeid();
        if (hasFlag(AppearanceFlag::CyclopediaItem))
            flagData.cyclopediaClientId = flags.cyclopediaitem().cyclopedia_type();
    }
}

uint32_t Appearance::getFirstSpriteId() const
{
    return getSpriteInfo().spriteIds.at(0);
}

const SpriteInfo &Appearance::getSpriteInfo(size_t frameGroup) const
{
    if (std::holds_alternative<SpriteInfo>(appearanceData))
    {
        return std::get<SpriteInfo>(appearanceData);
    }
    else
    {
        return std::get<std::vector<FrameGroup>>(appearanceData).at(frameGroup).spriteInfo;
    }
}

const SpriteInfo &Appearance::getSpriteInfo() const
{
    return getSpriteInfo(0);
}

size_t Appearance::frameGroupCount() const
{
    if (std::holds_alternative<SpriteInfo>(appearanceData))
    {
        return 1;
    }

    return std::get<std::vector<FrameGroup>>(appearanceData).size();
}