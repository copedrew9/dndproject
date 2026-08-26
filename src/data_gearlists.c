/* data_gearlists.c -- the small reference tables that round out an inventory.
 *
 * These are the parts of chapter 5 that are not really equipment: the
 * trinket a character may start with, the lifestyle they keep between
 * adventures, and the prices of food, lodging, services and spellcasting.
 * None of them belong in the item catalogue -- they have no weight and are
 * not carried -- but a player pricing a journey wants them to hand.
 */
#include "data.h"

/* -------------------------------------------------------------- trinkets */

const char *const TRINKETS[] = {
/*  1 */ "A shard of obsidian that always feels warm to the touch",
/*  2 */ "A dragon's bony talon hanging from a plain leather necklace",
/*  3 */ "A pair of old socks",
/*  4 */ "A blank book whose pages refuse to hold ink, chalk, graphite, or "
         "any other substance or marking",
/*  5 */ "A silver badge in the shape of a five-pointed star",
/*  6 */ "A knife that belonged to a relative",
/*  7 */ "A glass vial filled with nail clippings",
/*  8 */ "A rectangular metal device with two tiny metal cups on one end "
         "that throws sparks when wet",
/*  9 */ "A white, sequined glove sized for a human",
/* 10 */ "A vest with one hundred tiny pockets",
/* 11 */ "A small, weightless stone block",
/* 12 */ "A tiny sketch portrait of a goblin",
/* 13 */ "An empty glass vial that smells of perfume when opened",
/* 14 */ "A gemstone that looks like a lump of coal when examined by anyone "
         "but you",
/* 15 */ "A scrap of cloth from an old banner",
/* 16 */ "A rank insignia from a lost legionnaire",
/* 17 */ "A tiny silver bell without a clapper",
/* 18 */ "A mechanical canary inside a gnomish lamp",
/* 19 */ "A tiny chest carved to look like it has numerous feet on the "
         "bottom",
/* 20 */ "A dead sprite inside a clear glass bottle",
/* 21 */ "A metal can that has no opening but sounds as if it is filled with "
         "liquid, sand, spiders, or broken glass (your choice)",
/* 22 */ "A glass orb filled with water, in which swims a clockwork goldfish",
/* 23 */ "A silver spoon with an M engraved on the handle",
/* 24 */ "A whistle made from gold-colored wood",
/* 25 */ "A dead scarab beetle the size of your hand",
/* 26 */ "A mummified goblin hand",
/* 27 */ "A piece of crystal that faintly glows in the moonlight",
/* 28 */ "A gold coin minted in an unknown land",
/* 29 */ "A diary written in a language you don't know",
/* 30 */ "A brass ring that never tarnishes",
/* 31 */ "An old chess piece made from glass",
/* 32 */ "A pair of knucklebone dice, each with a skull symbol on the side "
         "that would normally show six pips",
/* 33 */ "A small idol depicting a nightmarish creature that gives you "
         "unsettling dreams when you sleep near it",
/* 34 */ "A rope necklace from which dangles four mummified elf fingers",
/* 35 */ "The deed for a parcel of land in a realm unknown to you",
/* 36 */ "A 1-ounce block made from an unknown material",
/* 37 */ "A small cloth doll skewered with needles",
/* 38 */ "A tooth from an unknown beast",
/* 39 */ "An enormous scale, perhaps from a dragon",
/* 40 */ "A bright green feather",
/* 41 */ "An old divination card bearing your likeness",
/* 42 */ "A glass orb filled with moving smoke",
/* 43 */ "A 1-pound egg with a bright red shell",
/* 44 */ "A pipe that blows bubbles",
/* 45 */ "A glass jar containing a weird bit of flesh floating in pickling "
         "fluid",
/* 46 */ "A tiny gnome-crafted music box that plays a song you dimly "
         "remember from your childhood",
/* 47 */ "A small wooden statuette of a smug halfling",
/* 48 */ "A brass orb etched with strange runes",
/* 49 */ "A multicolored stone disk",
/* 50 */ "A tiny silver icon of a raven",
/* 51 */ "A bag containing forty-seven humanoid teeth, one of which is "
         "rotten",
/* 52 */ "Two toy soldiers, one with a missing head",
/* 53 */ "A small box filled with different-sized buttons",
/* 54 */ "A candle that can't be lit",
/* 55 */ "A tiny cage with no door",
/* 56 */ "An old key",
/* 57 */ "An indecipherable treasure map",
/* 58 */ "A hilt from a broken sword",
/* 59 */ "A rabbit's foot",
/* 60 */ "A glass eye",
/* 61 */ "A cameo carved in the likeness of a hideous person",
/* 62 */ "A silver skull the size of a coin",
/* 63 */ "An alabaster mask",
/* 64 */ "A pyramid of sticky black incense that smells very bad",
/* 65 */ "A nightcap that, when worn, gives you pleasant dreams",
/* 66 */ "A single caltrop made from bone",
/* 67 */ "A gold monocle frame without the lens",
/* 68 */ "A 1-inch cube, each side painted a different color",
/* 69 */ "A crystal knob from a door",
/* 70 */ "A small packet filled with pink dust",
/* 71 */ "A fragment of a beautiful song, written as musical notes on two "
         "pieces of parchment",
/* 72 */ "A silver teardrop earring made from a real teardrop",
/* 73 */ "The shell of an egg painted with scenes of human misery in "
         "disturbing detail",
/* 74 */ "A fan that, when unfolded, shows a sleeping cat",
/* 75 */ "A set of bone pipes",
/* 76 */ "A four-leaf clover pressed inside a book discussing manners and "
         "etiquette",
/* 77 */ "A sheet of parchment upon which is drawn a complex mechanical "
         "contraption",
/* 78 */ "An ornate scabbard that fits no blade you have found so far",
/* 79 */ "An invitation to a party where a murder happened",
/* 80 */ "A bronze pentacle with an etching of a rat's head in its center",
/* 81 */ "A purple handkerchief embroidered with the name of a powerful "
         "archmage",
/* 82 */ "Half of a floorplan for a temple, castle, or some other structure",
/* 83 */ "A bit of folded cloth that, when unfolded, turns into a stylish "
         "cap",
/* 84 */ "A receipt of deposit at a bank in a far-flung city",
/* 85 */ "A diary with seven missing pages",
/* 86 */ "An empty silver snuffbox bearing an inscription on the surface "
         "that says \"dreams\"",
/* 87 */ "An iron holy symbol devoted to an unknown god",
/* 88 */ "A book that tells the story of a legendary hero's rise and fall, "
         "with the last chapter missing",
/* 89 */ "A vial of dragon blood",
/* 90 */ "An ancient arrow of elven design",
/* 91 */ "A needle that never bends",
/* 92 */ "An ornate brooch of dwarven design",
/* 93 */ "An empty wine bottle bearing a pretty label that says, \"The "
         "Wizard of Wines Winery, Red Dragon Crush, 331422-W\"",
/* 94 */ "A mosaic tile with a multicolored, glazed surface",
/* 95 */ "A petrified mouse",
/* 96 */ "A black pirate flag adorned with a dragon's skull and crossbones",
/* 97 */ "A tiny mechanical crab or spider that moves about when it's not "
         "being observed",
/* 98 */ "A glass jar containing lard with a label that reads, \"Griffon "
         "Grease\"",
/* 99 */ "A wooden box with a ceramic bottom that holds a living worm with a "
         "head on each end of its body",
/*100 */ "A metal urn containing the ashes of a hero",
};
const int TRINKET_COUNT = (int)(sizeof(TRINKETS) / sizeof(TRINKETS[0]));

/* ------------------------------------------------------------- lifestyles */

const Lifestyle LIFESTYLES[] = {
{ "Wretched", 0,
  "No home at all. You shelter in barns and crates and rely on the good "
  "graces of others. Violence, disease and hunger follow you, and you are "
  "beneath the notice of most people." },
{ "Squalid", 10,
  "A leaky stable, a mud-floored hut or a vermin-ridden boarding house. "
  "You have shelter from the elements but live in a desperate and often "
  "violent place." },
{ "Poor", 20,
  "Shabby surroundings and threadbare clothing, in a warren of cheap "
  "tenements. Most people treat you with contempt, and the law is unlikely "
  "to be on your side." },
{ "Modest", 100,
  "Out of the slums, in an older part of town, renting a room or living "
  "over a shop. You are not fashionable, but you are respectable enough for "
  "soldiers, labourers and families." },
{ "Comfortable", 200,
  "Fine clothing and no trouble paying for maintenance. You live in a "
  "small cottage in a middle-class neighbourhood or a comfortable "
  "apartment, and can associate with merchants and officials." },
{ "Wealthy", 400,
  "A life of luxury, though not the highest circles of society. A spacious "
  "home, a staff, and connections among the rich and powerful -- along "
  "with the attention of thieves." },
{ "Aristocratic", 1000,
  "A life of great comfort with access to the finest society, at 10 gp a "
  "day and often much more. Your money opens doors, and it also draws "
  "attention from those who would use or rob you." },
};
const int LIFESTYLE_COUNT = (int)(sizeof(LIFESTYLES) / sizeof(LIFESTYLES[0]));

/* ---------------------------------------------- food, lodging and services */

const PriceEntry SERVICES[] = {
/* lodging and food, all per day unless noted */
{ "Inn stay, squalid (per day)",      7 },
{ "Inn stay, poor (per day)",        10 },
{ "Inn stay, modest (per day)",      50 },
{ "Inn stay, comfortable (per day)", 80 },
{ "Inn stay, wealthy (per day)",    200 },
{ "Inn stay, aristocratic (per day)", 400 },
{ "Meals, squalid (per day)",         3 },
{ "Meals, poor (per day)",            6 },
{ "Meals, modest (per day)",         30 },
{ "Meals, comfortable (per day)",    50 },
{ "Meals, wealthy (per day)",        80 },
{ "Meals, aristocratic (per day)",  200 },
{ "Ale, gallon",                     20 },
{ "Ale, mug",                         4 },
{ "Banquet (per person)",          1000 },
{ "Bread, loaf",                      2 },
{ "Cheese, hunk",                    10 },
{ "Meat, chunk",                     30 },
{ "Wine, common (pitcher)",          20 },
{ "Wine, fine (bottle)",           1000 },
/* services */
{ "Coach cab, between towns (per mile)",  3 },
{ "Coach cab, within a city",             1 },
{ "Hireling, skilled (per day)",        200 },
{ "Hireling, untrained (per day)",        2 },
{ "Messenger (per mile)",                 2 },
{ "Road or gate toll",                    1 },
{ "Ship's passage (per mile)",            1 },
{ "Stabling (per day)",                  50 },
};
const int SERVICE_COUNT = (int)(sizeof(SERVICES) / sizeof(SERVICES[0]));

/* Spellcasting bought from an NPC. The caster's fee is on top of any costly
 * material component, which the buyer must also provide. */
const PriceEntry SPELLCASTING_SERVICES[] = {
{ "Cantrip or 1st-level spell (cast for you)",   3000 },
{ "2nd-level spell",                             5000 },
{ "3rd-level spell",                            10000 },
{ "4th-level spell",                            20000 },
{ "5th-level spell",                            50000 },
{ "Raise dead (5th level, plus 500 gp diamond)", 125000 },
};
const int SPELLCASTING_SERVICE_COUNT =
    (int)(sizeof(SPELLCASTING_SERVICES) / sizeof(SPELLCASTING_SERVICES[0]));
