/* data_itemtext.c -- what each item actually does.
 *
 * The catalogue in data_equipment.c carries the numbers: cost, weight,
 * Armor Class, damage. This file carries everything else, so that looking an
 * item up tells you something useful whether or not it has a stat line. Gear
 * entries summarise the mechanical effect from the PHB's equipment chapter;
 * tool entries follow Xanathar's, which lists what each kit contains and
 * which ability checks proficiency with it can help.
 *
 * Notes are keyed by item name rather than stored in the catalogue row, so
 * the two tables can be edited independently.
 */
#include "data.h"
#include <string.h>

const ItemNote ITEM_NOTES[] = {
/* ------------------------------- armour ---------------------------------- */
{ "Padded armor",
  "Light armor: add your full Dexterity modifier to AC. Donning takes 1 "
  "minute, doffing 1 minute. The quilting makes you noisy: disadvantage on "
  "Stealth." },
{ "Leather armor",
  "Light armor: add your full Dexterity modifier to AC. Don 1 minute, "
  "doff 1 minute." },
{ "Studded leather armor",
  "Light armor: add your full Dexterity modifier to AC. Don 1 minute, "
  "doff 1 minute." },
{ "Hide armor",
  "Medium armor: add up to +2 from Dexterity. Don 5 minutes, doff 1 minute." },
{ "Chain shirt",
  "Medium armor worn under clothing: add up to +2 from Dexterity. Don 5 "
  "minutes, doff 1 minute." },
{ "Scale mail",
  "Medium armor: add up to +2 from Dexterity. Don 5 minutes, doff 1 minute. "
  "Disadvantage on Stealth." },
{ "Breastplate",
  "Medium armor that leaves the limbs free: add up to +2 from Dexterity. "
  "Don 5 minutes, doff 1 minute." },
{ "Half plate",
  "Medium armor: add up to +2 from Dexterity. Don 5 minutes, doff 5 minutes. "
  "Disadvantage on Stealth." },
{ "Ring mail",
  "Heavy armor: Dexterity does not apply. Don 10 minutes, doff 5 minutes. "
  "Disadvantage on Stealth." },
{ "Chain mail",
  "Heavy armor: Dexterity does not apply. Requires Strength 13 or your speed "
  "drops by 10 feet. Don 10 minutes, doff 5 minutes. Disadvantage on "
  "Stealth." },
{ "Splint armor",
  "Heavy armor: Dexterity does not apply. Requires Strength 15 or your speed "
  "drops by 10 feet. Don 10 minutes, doff 5 minutes. Disadvantage on "
  "Stealth." },
{ "Plate armor",
  "Heavy armor: Dexterity does not apply. Requires Strength 15 or your speed "
  "drops by 10 feet. Don 10 minutes, doff 5 minutes. Disadvantage on "
  "Stealth." },
{ "Shield",
  "Wielded in one hand for +2 AC. Donning or doffing it takes an action. "
  "You gain no benefit from more than one shield at a time." },

/* -------------------------------- gear ----------------------------------- */
{ "Acid (vial)",
  "As an action, throw it up to 20 feet as a ranged attack against a creature "
  "or object; on a hit it deals 2d6 acid damage." },
{ "Alchemist's fire (flask)",
  "As an action, throw it up to 20 feet as a ranged attack. On a hit the "
  "target takes 1d4 fire damage at the start of each of its turns until "
  "someone uses an action to make a DC 10 Dexterity check to put it out." },
{ "Antitoxin (vial)",
  "Drinking it grants advantage on saving throws against poison for 1 hour. "
  "It does nothing for undead or constructs." },
{ "Ball bearings (bag of 1,000)",
  "As an action, spill them across a level 10-foot square. A creature moving "
  "through must make a DC 10 Dexterity save or fall prone; moving at half "
  "speed avoids the save." },
{ "Caltrops (bag of 20)",
  "As an action, scatter them over a 5-foot square. A creature entering must "
  "make a DC 15 Dexterity save or take 1 piercing damage and stop moving, "
  "its speed reduced by 10 feet until healed." },
{ "Block and tackle",
  "Pulleys and a hook that let you hoist up to four times the weight you "
  "could normally lift." },
{ "Book",
  "A hundred pages of poetry, history, lore or diagrams. Studying one can "
  "grant advantage on an Intelligence check about its subject, at the DM's "
  "discretion." },
{ "Candle",
  "Burns for 1 hour, shedding bright light in a 5-foot radius and dim light "
  "for 5 feet beyond." },
{ "Chain (10 feet)",
  "Has 10 hit points and can be burst with a DC 20 Strength check." },
{ "Climber's kit",
  "Pitons, boot tips, gloves and a harness. As an action you can anchor "
  "yourself, after which you cannot fall more than 25 feet from that point "
  "and cannot move more than 25 feet from it without unanchoring." },
{ "Component pouch",
  "A watertight belt pouch holding every material component your spells need, "
  "except those with a listed cost. It can replace a spellcasting focus." },
{ "Crowbar",
  "Grants advantage on Strength checks where its leverage applies." },
{ "Crystal (arcane focus)",
  "An arcane focus: a sorcerer, warlock or wizard can use it in place of the "
  "material components of their spells, except those with a listed cost." },
{ "Orb (arcane focus)", "An arcane focus for a sorcerer, warlock or wizard." },
{ "Rod (arcane focus)", "An arcane focus for a sorcerer, warlock or wizard." },
{ "Staff (arcane focus)",
  "An arcane focus for a sorcerer, warlock or wizard; it also functions as a "
  "quarterstaff." },
{ "Wand (arcane focus)", "An arcane focus for a sorcerer, warlock or wizard." },
{ "Sprig of mistletoe (druidic focus)",
  "A druidic focus: a druid or ranger can use it in place of the material "
  "components of their spells, except those with a listed cost." },
{ "Wooden staff (druidic focus)",
  "A druidic focus; it also functions as a quarterstaff." },
{ "Yew wand (druidic focus)", "A druidic focus for a druid or ranger." },
{ "Amulet (holy symbol)",
  "A holy symbol: a cleric or paladin can use it in place of the material "
  "components of their spells. It must be held, worn visibly, or borne on a "
  "shield." },
{ "Emblem (holy symbol)",
  "A holy symbol emblazoned on a shield or worn visibly." },
{ "Reliquary (holy symbol)",
  "A holy symbol in the form of a small box holding a sacred relic." },
{ "Fishing tackle",
  "A rod, line, bobbers, hooks, sinkers, lures and netting, for catching "
  "fish." },
{ "Healer's kit",
  "Ten uses. As an action, spend one to stabilise a creature at 0 hit points "
  "without needing a Wisdom (Medicine) check." },
{ "Holy water (flask)",
  "As an action, splash it on a creature within 5 feet or throw it up to 20 "
  "feet as a ranged attack. A fiend or undead takes 2d6 radiant damage." },
{ "Hunting trap",
  "Set as an action. A creature stepping on it makes a DC 13 Dexterity save "
  "or takes 1d4 piercing damage and stops moving; escaping needs a DC 13 "
  "Strength check." },
{ "Lamp",
  "Bright light in a 15-foot radius and dim light for 30 feet beyond. Burns "
  "6 hours on a pint of oil." },
{ "Lantern, bullseye",
  "Bright light in a 60-foot cone and dim light for 60 feet beyond. Burns 6 "
  "hours on a pint of oil." },
{ "Lantern, hooded",
  "Bright light in a 30-foot radius and dim light for 30 feet beyond. Burns "
  "6 hours on a pint of oil; as an action you can lower the hood to dim it "
  "to a 5-foot radius." },
{ "Lock",
  "Comes with one key. Without it, picking the lock needs thieves' tools and "
  "a successful DC 15 Dexterity check." },
{ "Magnifying glass",
  "Doubles the apparent size of small objects, and can start a fire in "
  "sunlight given tinder and about 5 minutes." },
{ "Manacles",
  "Bind a Small or Medium creature. Escaping needs a DC 20 Dexterity check; "
  "breaking them a DC 20 Strength check. Each set comes with one key." },
{ "Mess kit",
  "A tin box holding a cup and cutlery, with one side serving as a pan and "
  "the other as a plate." },
{ "Oil (flask)",
  "A pint. Thrown as an improvised weapon it shatters and coats a creature, "
  "which then takes 5 fire damage if set alight; or pour it on the ground to "
  "cover a 5-foot square." },
{ "Poison, basic (vial)",
  "Apply it to one slashing or piercing weapon or up to three pieces of "
  "ammunition. A creature hit makes a DC 10 Constitution save or takes 1d4 "
  "poison damage. It dries after 1 minute." },
{ "Potion of healing",
  "Drinking or administering it takes an action and restores 2d4 + 2 hit "
  "points." },
{ "Pouch",
  "Holds up to 20 sling bullets, 50 blowgun needles, or a fifth of a cubic "
  "foot of gear." },
{ "Quiver", "Holds up to 20 arrows." },
{ "Ram, portable",
  "Grants +4 on Strength checks to break down a door, and another character "
  "helping adds advantage." },
{ "Rations (1 day)",
  "Dry food for travel: jerky, dried fruit, hardtack and nuts." },
{ "Rope, hempen (50 feet)",
  "Has 2 hit points and can be burst with a DC 17 Strength check." },
{ "Rope, silk (50 feet)",
  "Lighter than hemp; has 2 hit points and can be burst with a DC 17 "
  "Strength check." },
{ "Scale, merchant's",
  "A balance, pans and weights, for measuring coins, gems and trade goods." },
{ "Spellbook",
  "A leather-bound tome of 100 blank vellum pages, in which a wizard records "
  "the spells they know." },
{ "Spyglass", "Objects viewed through it appear twice their size." },
{ "Tent, two-person", "A portable canvas shelter that sleeps two." },
{ "Tinderbox",
  "Flint, steel and tinder. Lighting a torch takes an action; lighting any "
  "other fire takes a minute." },
{ "Torch",
  "Burns for 1 hour, giving bright light in a 20-foot radius and dim light "
  "for 20 feet beyond. A melee attack with a lit torch deals 1 fire damage." },

/* -------------------------------- tools ---------------------------------- */
{ "Alchemist's supplies",
  "Beakers, a frame, a mortar and pestle, and pouches of reagents. Helps with "
  "Arcana checks on alchemical matters and Investigation of chemical traces, "
  "and lets you make acid, alchemist's fire, oil, perfume and soap." },
{ "Brewer's supplies",
  "A large glass jar, tubing, a siphon and hydrometer. Helps with History "
  "checks about famous brews, Medicine checks to stave off infection, and "
  "Persuasion when a drink smooths the way. Purifies water." },
{ "Calligrapher's supplies",
  "Ink, a variety of pens and parchment. Helps with Arcana checks on runes "
  "and scripts and History checks on old documents, and lets you produce "
  "convincing writing." },
{ "Carpenter's tools",
  "A saw, hammer, nails, hatchet, square, ruler, adze, plane and chisel. "
  "Helps with History checks about structures, Investigation of wooden "
  "construction, and Perception to spot a hidden compartment." },
{ "Cartographer's tools",
  "Quills, ink, parchment, calipers, compasses and rulers. Helps with Arcana "
  "and History checks about maps, Nature checks to judge terrain, and "
  "Survival to avoid getting lost." },
{ "Cobbler's tools",
  "A hammer, awl, knife, shoe stand, thread and leather. Helps with "
  "Arcana checks on magical footwear, History checks on a shoe's origin, and "
  "Investigation of where someone has walked." },
{ "Cook's utensils",
  "A metal pot, knives, forks, a stirring spoon and ladle. Helps with History "
  "checks on regional dishes, Medicine to prepare food for the sick, and "
  "Survival to make the most of foraged food." },
{ "Glassblower's tools",
  "A blowpipe, a small marver, blocks and tweezers. Helps with Arcana checks "
  "on magical glass objects, History checks about a piece's origin, and "
  "Investigation of flaws or hidden contents." },
{ "Jeweler's tools",
  "A small saw and hammer, files, pliers and tweezers. Helps with Arcana "
  "checks on gem-based magic, Investigation to appraise gems, and lets you "
  "identify a stone's value." },
{ "Leatherworker's tools",
  "A knife, mallet, edger, hole punch, thread and leather. Helps with Arcana "
  "checks on leather magic items, Investigation of leatherwork, and repairs "
  "to leather armor." },
{ "Mason's tools",
  "A trowel, hammer, chisel, brushes and square. Helps with History checks on "
  "stonework, Investigation to find a weak point, and Perception to spot "
  "unusual masonry." },
{ "Painter's supplies",
  "An easel, canvas, brushes, paints and charcoal. Helps with Arcana and "
  "History checks about symbols and art, Investigation of a scene, and "
  "Perception to spot details others miss." },
{ "Potter's tools",
  "Needles, ribs, scrapers, a knife and calipers. Helps with History checks "
  "on a piece's age and origin, Investigation of shards, and Perception to "
  "find something hidden in a vessel." },
{ "Smith's tools",
  "Hammers, tongs, charcoal, rags and a whetstone. Helps with Arcana checks "
  "on magic weapons and armor, History checks on metal objects, and repairs "
  "to metal armor and weapons." },
{ "Tinker's tools",
  "Files, hammers, a small saw, pliers, whetstone, thread and needles. Helps "
  "with History and Investigation checks on devices, and lets you repair "
  "damaged objects and build simple contraptions." },
{ "Weaver's tools",
  "Thread, needles and scraps of cloth. Helps with Arcana checks on magical "
  "garments, History on a garment's origin, and Investigation of a bolt of "
  "cloth. Repairs and tailors clothing." },
{ "Woodcarver's tools",
  "A knife, gouge and small saw. Helps with Arcana checks on magic staffs and "
  "wands, History checks about wooden objects, and Nature to identify a "
  "wood. Makes arrows and wooden instruments." },
{ "Disguise kit",
  "Cosmetics, hair dye, small props and clothing. Helps with Deception to "
  "pass as someone else, Intimidation by looking the part, Performance in "
  "costume, and Persuasion when appearance matters." },
{ "Forgery kit",
  "Inks, papers, seals, sealing wax, gold and silver leaf. Helps with "
  "Deception when passing off a document and Arcana checks on written magic. "
  "Lets you forge documents and duplicate seals and signatures." },
{ "Herbalism kit",
  "Pouches for herbs, clippers, gloves, a mortar and pestle and glass jars. "
  "Helps with Arcana on plant-based magic, Investigation of overgrown areas, "
  "Medicine to treat wounds, and Nature and Survival in the wild. Lets you "
  "make antitoxin and potions of healing." },
{ "Navigator's tools",
  "A sextant, compass, calipers, ruler, parchment, ink and quill. Helps with "
  "Survival to avoid getting lost and Perception to spot land or a landmark, "
  "and lets you plot a course at sea." },
{ "Poisoner's kit",
  "Glass vials, a mortar and pestle, chemicals and a glass stirring rod. "
  "Helps with Investigation and Perception to spot poison, Medicine to treat "
  "it, Nature to identify a poisonous creature, and Survival to forage." },
{ "Thieves' tools",
  "A file, lock picks, a small mirror on a handle, narrow-bladed scissors and "
  "pliers. Needed to pick locks and disarm traps, and helps with History "
  "checks on notorious heists and Investigation and Perception on traps and "
  "security." },
{ "Dice set",
  "A gaming set. Proficiency covers one type of game; it helps with History "
  "checks about the game's history, Insight to read an opponent, and Sleight "
  "of Hand to cheat." },
{ "Playing card set",
  "A gaming set. Proficiency covers one type of game; it helps with History, "
  "Insight to read an opponent, and Sleight of Hand to cheat." },
{ "Bagpipes",
  "A musical instrument. Proficiency lets you add your proficiency bonus to "
  "Performance checks with it, and helps with History checks on songs and "
  "Perception to pick out a tune." },
{ "Drum", "A musical instrument; see Bagpipes." },
{ "Dulcimer", "A musical instrument; see Bagpipes." },
{ "Flute", "A musical instrument; see Bagpipes." },
{ "Lute", "A musical instrument; see Bagpipes." },
{ "Lyre", "A musical instrument; see Bagpipes." },
{ "Horn", "A musical instrument; see Bagpipes." },
{ "Pan flute", "A musical instrument; see Bagpipes." },
{ "Shawm", "A musical instrument; see Bagpipes." },
{ "Viol", "A musical instrument; see Bagpipes." },

/* ------------------------------- mounts ---------------------------------- */
{ "Camel",   "Speed 50 ft., carrying capacity 480 lb." },
{ "Donkey or mule", "Speed 40 ft., carrying capacity 420 lb." },
{ "Draft horse", "Speed 40 ft., carrying capacity 540 lb." },
{ "Elephant", "Speed 40 ft., carrying capacity 1,320 lb." },
{ "Mastiff",  "Speed 40 ft., carrying capacity 195 lb. Small enough to fit "
              "where a horse cannot." },
{ "Pony",     "Speed 40 ft., carrying capacity 225 lb." },
{ "Riding horse", "Speed 60 ft., carrying capacity 480 lb." },
{ "Warhorse", "Speed 60 ft., carrying capacity 540 lb. Trained for battle; a "
              "draft horse counts as a warhorse for carrying capacity." },
{ "Saddle, riding",
  "A riding saddle grants advantage on any check made to remain mounted." },
{ "Saddlebags", "Holds a third of a cubic foot or 8 pounds of gear." },
{ "Feed (per day)", "A day's fodder for one mount or beast of burden." },
};
const int ITEM_NOTE_COUNT = (int)(sizeof(ITEM_NOTES) / sizeof(ITEM_NOTES[0]));

const char *item_notes(const char *name)
{
    int i;
    for (i = 0; i < ITEM_NOTE_COUNT; i++) {
        if (strcmp(ITEM_NOTES[i].item, name) == 0) return ITEM_NOTES[i].text;
    }
    return NULL;
}

/* Weapon properties, so a weapon's listed properties can be explained. */
const ItemNote WEAPON_PROPERTIES[] = {
{ "Ammunition",
  "You need ammunition to make a ranged attack. Drawing it is part of the "
  "attack; at the end of a battle you can recover half the ammunition spent." },
{ "Finesse",
  "Use either Strength or Dexterity for the attack and damage rolls, the "
  "same modifier for both." },
{ "Heavy",
  "Small creatures have disadvantage on attack rolls with it, because its "
  "size and bulk are too much for them." },
{ "Light",
  "Small and easy to handle, making it suitable for fighting with two "
  "weapons." },
{ "Loading",
  "You can fire only one piece of ammunition per action, bonus action or "
  "reaction, however many attacks you could normally make." },
{ "Range",
  "Two numbers: attacks beyond the first are at disadvantage, and you cannot "
  "attack beyond the second at all." },
{ "Reach",
  "Adds 5 feet to your reach when you attack with it, and to the space you "
  "threaten for opportunity attacks." },
{ "Special",
  "The weapon has a rule of its own. A lance has disadvantage within 5 feet "
  "and needs two hands unless you are mounted; a net restrains a Large or "
  "smaller creature on a hit and has no damage." },
{ "Thrown",
  "You can throw it to make a ranged attack, using the same ability modifier "
  "you would use in melee." },
{ "Two-Handed",
  "The weapon requires two hands when you attack with it." },
{ "Versatile",
  "Can be used with one or two hands; the larger damage die in brackets "
  "applies when wielded in two hands." },
};
const int WEAPON_PROPERTY_COUNT =
    (int)(sizeof(WEAPON_PROPERTIES) / sizeof(WEAPON_PROPERTIES[0]));
