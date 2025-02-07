#include "Items.h"
#include "Misc.h"

#include <Interfaces.h>

namespace inventory_changer::game_integration
{

void Items::getMusicKits(game_items::Storage& storage)
{
    for (const auto& node : itemSchema.musicKits) {
        const auto musicKit = node.value;
        if (musicKit->id == 1 || musicKit->id == 2)
            continue;

        const auto musicName = interfaces->localize->findSafe(musicKit->nameLocalized);
        storage.addMusic(musicKit->id, game_items::ItemName{ toUtf8.convertUnicodeToAnsi(musicName), toUpperConverter.toUpper(musicName) }, musicKit->inventoryImage);
    }
}

void Items::getStickers(game_items::Storage& storage)
{
    const auto& stickerMap = itemSchema.stickerKits;
    for (const auto& node : stickerMap) {
        const auto stickerKit = node.value;
        if (stickerKit->id == 0)
            continue;

        const auto name = std::string_view{ stickerKit->name.data(), static_cast<std::size_t>(stickerKit->name.length - 1) };
        const auto isPatch = name.starts_with("patch") || name.starts_with("stockh2021_teampatch");
        const auto isGraffiti = !isPatch && (name.starts_with("spray") || name.ends_with("graffiti"));
        const auto isSticker = !isPatch && !isGraffiti;

        if (isSticker) {
            const auto isGolden = name.ends_with("gold");
            const auto stickerName = interfaces->localize->findSafe(stickerKit->id != 242 ? stickerKit->itemName.data() : "StickerKit_dhw2014_teamdignitas_gold");
            storage.addSticker(stickerKit->id, game_items::ItemName{ toUtf8.convertUnicodeToAnsi(stickerName), toUpperConverter.toUpper(stickerName) }, static_cast<EconRarity>(stickerKit->rarity), stickerKit->inventoryImage.data(), stickerKit->tournamentID, static_cast<TournamentTeam>(stickerKit->tournamentTeamID), stickerKit->tournamentPlayerID, isGolden);
        } else if (isPatch) {
            const auto patchName = interfaces->localize->findSafe(stickerKit->itemName.data());
            storage.addPatch(stickerKit->id, game_items::ItemName{ toUtf8.convertUnicodeToAnsi(patchName), toUpperConverter.toUpper(patchName) }, static_cast<EconRarity>(stickerKit->rarity), stickerKit->inventoryImage.data());
        } else if (isGraffiti) {
            const auto paintName = interfaces->localize->findSafe(stickerKit->itemName.data());
            storage.addGraffiti(stickerKit->id, game_items::ItemName{ toUtf8.convertUnicodeToAnsi(paintName), toUpperConverter.toUpper(paintName) }, static_cast<EconRarity>(stickerKit->rarity), stickerKit->inventoryImage.data());
        }
    }
}

namespace
{

struct KitWeapon {
    KitWeapon(int paintKit, WeaponId weaponId, const char* iconPath) noexcept : paintKit{ paintKit }, weaponId{ weaponId }, iconPath{ iconPath } {}
    int paintKit;
    WeaponId weaponId;
    const char* iconPath;
};

[[nodiscard]] std::vector<KitWeapon> getKitsWeapons(const UtlMap<std::uint64_t, AlternateIconData>& alternateIcons)
{
    std::vector<KitWeapon> kitsWeapons;
    kitsWeapons.reserve(alternateIcons.numElements);

    for (const auto& node : alternateIcons) {
        // https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/shared/econ/econ_item_schema.cpp#L325-L329
        if (const auto encoded = node.key; (encoded & 3) == 0)
            kitsWeapons.emplace_back(int((encoded & 0xFFFF) >> 2), WeaponId(encoded >> 16), node.value.simpleName.data());
    }
    std::ranges::sort(kitsWeapons, {}, &KitWeapon::paintKit);
    return kitsWeapons;
}

}

void Items::getSkinsAndGloves(game_items::Storage& storage)
{
    const auto kitsWeapons = getKitsWeapons(itemSchema.alternateIcons);

    for (const auto& node : itemSchema.paintKits) {
        const auto paintKit = node.value;

        if (paintKit->id == 0 || paintKit->id == 9001) // ignore workshop_default
            continue;

        const auto paintKitName = interfaces->localize->findSafe(paintKit->itemName.data());
        storage.addPaintKit(paintKit->id, game_items::ItemName{ toUtf8.convertUnicodeToAnsi(paintKitName), toUpperConverter.toUpper(paintKitName) }, paintKit->wearRemapMin, paintKit->wearRemapMax);

        const auto isGlove = (paintKit->id >= 10000);
        for (auto it = std::ranges::lower_bound(kitsWeapons, paintKit->id, {}, &KitWeapon::paintKit); it != kitsWeapons.end() && it->paintKit == paintKit->id; ++it) {
            const auto itemDef = itemSchema.getItemDefinitionInterface(it->weaponId);
            if (!itemDef)
                continue;

            if (isGlove) {
                storage.addGlovesWithLastPaintKit(static_cast<EconRarity>(paintKit->rarity), it->weaponId, it->iconPath);
            } else {
                storage.addSkinWithLastPaintKit(static_cast<EconRarity>(std::clamp(itemDef->getRarity() + paintKit->rarity - 1, 0, (paintKit->rarity == 7) ? 7 : 6)), it->weaponId, it->iconPath);
            }
        }
    }
}

void Items::getOtherItems(game_items::Storage& storage)
{
    for (const auto& node : itemSchema.itemsSorted) {
        const auto item = node.value;
        const auto itemTypeName = std::string_view{ item->getItemTypeName() };
        const auto isCollectible = (itemTypeName == "#CSGO_Type_Collectible");
        const auto isOriginal = (item->getQuality() == 1);

        const auto inventoryImage = item->getInventoryImage();
        if (!inventoryImage)
            continue;

        const auto rarity = EconRarity{ item->getRarity() };

        if (const auto weaponID = item->getWeaponId(); itemTypeName == "#CSGO_Type_Knife" && rarity == EconRarity::Red) {
            storage.addVanillaKnife(weaponID, inventoryImage);
        } else if (isCollectible) {
            if (item->isServiceMedal()) {
                storage.addServiceMedal(rarity, item->getServiceMedalYear(), weaponID, inventoryImage);
            } else if (item->isTournamentCoin()) {
                storage.addTournamentCoin(rarity, weaponID, static_cast<std::uint8_t>(item->getTournamentEventID()), static_cast<std::uint16_t>(item->getStickerID()), inventoryImage);
            } else {
                storage.addCollectible(rarity, weaponID, isOriginal, inventoryImage);
            }
        } else if (itemTypeName == "#CSGO_Tool_Name_TagTag") {
            storage.addNameTag(rarity, weaponID, inventoryImage);
        } else if (item->isPatchable()) {
            storage.addAgent(rarity, weaponID, inventoryImage);
        } else if (itemTypeName == "#CSGO_Type_WeaponCase" && item->hasCrateSeries()) {
            const auto baseName = std::string_view{ item->getItemBaseName() };
            storage.addCase(rarity, weaponID, static_cast<std::uint16_t>(item->getCrateSeriesNumber()), static_cast<std::uint8_t>(item->getTournamentEventID()), getTournamentMapOfSouvenirPackage(baseName), baseName.find("promo") != std::string_view::npos, inventoryImage);
        } else if (itemTypeName == "#CSGO_Tool_WeaponCase_KeyTag") {
            storage.addCaseKey(rarity, weaponID, inventoryImage);
        } else if (const auto tool = item->getEconTool()) {
            if (std::strcmp(tool->typeName, "season_pass") == 0)
                storage.addOperationPass(rarity, weaponID, inventoryImage);
            else if (std::strcmp(tool->typeName, "stattrak_swap") == 0)
                storage.addStatTrakSwapTool(rarity, weaponID, inventoryImage);
            else if (std::strcmp(tool->typeName, "fantoken") == 0) {
                if (Helpers::isSouvenirToken(weaponID))
                    storage.addSouvenirToken(rarity, weaponID, item->getTournamentEventID(), inventoryImage);
                else
                    storage.addViewerPass(rarity, weaponID, item->getTournamentEventID(), inventoryImage);
            } else if (std::strcmp(tool->typeName, "casket") == 0) {
                storage.addStorageUnit(rarity, weaponID, inventoryImage);
            }
        } else if (item->isPaintable()) {
            storage.addVanillaSkin(weaponID, inventoryImage);
        }
    }
}

game_items::Storage createGameItemStorage(Items& items)
{
    game_items::Storage storage;
    items.getStickers(storage);
    items.getMusicKits(storage);
    items.getSkinsAndGloves(storage);
    items.getOtherItems(storage);
    return storage;
}

}
