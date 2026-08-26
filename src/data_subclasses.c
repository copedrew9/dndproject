/* data_subclasses.c -- every subclass, from the Player's Handbook,
 * Xanathar's Guide to Everything and Tasha's Cauldron of Everything.
 *
 * Entries are appended, never reordered, so the numeric ids that
 * data_features.c uses stay stable. A class finds its own subclasses by
 * scanning for its class_id (see subclasses_of), so they need not be
 * contiguous.
 *
 * Domain, oath, circle, patron and specialist spell lists were checked
 * against the books with tools/extract_subclass_spells.py.
 */
#include "data.h"

/* ------------------------------------------------------------- subclasses */

const SubclassData SUBCLASSES[] = {
    /* --- barbarian (0-1) --- */
    { 0, BOOK_PHB, "Path of the Berserker",
      "Fury in battle: frenzy grants a bonus attack at the cost of exhaustion.",
      "", "", "" },
    { 0, BOOK_PHB, "Path of the Totem Warrior",
      "A spirit animal guides you, granting resilience or ferocity.",
      "", "Totem spirit", "Bear|Eagle|Wolf" },

    /* --- bard (2-3) --- */
    { 1, BOOK_PHB, "College of Lore",
      "Knowledge and cutting words; extra skills and additional magical secrets.",
      "", "", "" },
    { 1, BOOK_PHB, "College of Valor",
      "A battle skald: martial training and inspiration that aids attacks.",
      "", "", "" },

    /* --- cleric (4-10) --- */
    { 2, BOOK_PHB, "Knowledge Domain",
      "The pursuit of learning; you read thoughts and borrow proficiencies.",
      "command, identify|augury, suggestion|nondetection, speak with dead|"
      "arcane eye, confusion|legend lore, scrying", "", "" },
    { 2, BOOK_PHB, "Life Domain",
      "Healing and vitality; your cures are more potent than others'.",
      "bless, cure wounds|lesser restoration, spiritual weapon|"
      "beacon of hope, revivify|death ward, guardian of faith|"
      "mass cure wounds, raise dead", "", "" },
    { 2, BOOK_PHB, "Light Domain",
      "Radiance and fire; you blind foes and shield allies with light.",
      "burning hands, faerie fire|flaming sphere, scorching ray|"
      "daylight, fireball|guardian of faith, wall of fire|"
      "flame strike, scrying", "", "" },
    { 2, BOOK_PHB, "Nature Domain",
      "The natural world; you command beasts and plants.",
      "animal friendship, speak with animals|barkskin, spike growth|"
      "plant growth, wind wall|dominate beast, grasping vine|"
      "insect plague, tree stride", "", "" },
    { 2, BOOK_PHB, "Tempest Domain",
      "Storm and thunder; you strike with maximised lightning and thunder.",
      "fog cloud, thunderwave|gust of wind, shatter|"
      "call lightning, sleet storm|control water, ice storm|"
      "destructive wave, insect plague", "", "" },
    { 2, BOOK_PHB, "Trickery Domain",
      "Deception and stealth; you create illusory duplicates and bless allies "
      "with stealth.",
      "charm person, disguise self|mirror image, pass without trace|"
      "blink, dispel magic|dimension door, polymorph|"
      "dominate person, modify memory", "", "" },
    { 2, BOOK_PHB, "War Domain",
      "Battle prowess; bonus attacks and divine strikes.",
      "divine favor, shield of faith|magic weapon, spiritual weapon|"
      "crusader's mantle, spirit guardians|freedom of movement, stoneskin|"
      "flame strike, hold monster", "", "" },

    /* --- druid (11-12) --- */
    { 3, BOOK_PHB, "Circle of the Land",
      "A druid of a particular terrain, with extra spells and recovery.",
      "", "Land type",
      "Arctic|Coast|Desert|Forest|Grassland|Mountain|Swamp|Underdark" },
    { 3, BOOK_PHB, "Circle of the Moon",
      "A shapeshifter: wild shape as a bonus action into fiercer forms.",
      "", "", "" },

    /* --- fighter (13-15) --- */
    { 4, BOOK_PHB, "Champion",
      "Simple, relentless martial excellence; improved critical hits.",
      "", "", "" },
    { 4, BOOK_PHB, "Battle Master",
      "Tactical manoeuvres fuelled by superiority dice.",
      "", "", "" },
    { 4, BOOK_PHB, "Eldritch Knight",
      "A fighter who weaves wizard magic into swordplay (third-caster).",
      "", "", "" },

    /* --- monk (16-18) --- */
    { 5, BOOK_PHB, "Way of the Open Hand",
      "Mastery of unarmed combat; manipulate a foe's ki.",
      "", "", "" },
    { 5, BOOK_PHB, "Way of Shadow",
      "A ninja of stealth and darkness, stepping between shadows.",
      "", "", "" },
    { 5, BOOK_PHB, "Way of the Four Elements",
      "Bend the elements to your will through ki-fuelled disciplines.",
      "", "", "" },

    /* --- paladin (19-21) --- */
    { 6, BOOK_PHB, "Oath of Devotion",
      "The classic knight in shining armour: honesty, courage, duty.",
      "protection from evil and good, sanctuary|"
      "lesser restoration, zone of truth|beacon of hope, dispel magic|"
      "freedom of movement, guardian of faith|commune, flame strike", "", "" },
    { 6, BOOK_PHB, "Oath of the Ancients",
      "A green knight preserving light and life in the world.",
      "ensnaring strike, speak with animals|moonbeam, misty step|"
      "plant growth, protection from energy|ice storm, stoneskin|"
      "commune with nature, tree stride", "", "" },
    { 6, BOOK_PHB, "Oath of Vengeance",
      "A dark avenger who punishes wrongdoers at any cost.",
      "bane, hunter's mark|hold person, misty step|"
      "haste, protection from energy|banishment, dimension door|"
      "hold monster, scrying", "", "" },

    /* --- ranger (22-23) --- */
    { 7, BOOK_PHB, "Hunter",
      "A monster slayer with tactics tuned to the prey you face.",
      "", "", "" },
    { 7, BOOK_PHB, "Beast Master",
      "You bond with an animal companion that fights alongside you.",
      "", "", "" },

    /* --- rogue (24-26) --- */
    { 8, BOOK_PHB, "Thief",
      "Fast hands, climbing and the use of magic items others cannot.",
      "", "", "" },
    { 8, BOOK_PHB, "Assassin",
      "Disguise, poison and devastating strikes against the unready.",
      "", "", "" },
    { 8, BOOK_PHB, "Arcane Trickster",
      "A rogue who enhances stealth and mischief with wizard magic "
      "(third-caster).",
      "", "", "" },

    /* --- sorcerer (27-28) --- */
    { 9, BOOK_PHB, "Draconic Bloodline",
      "Dragon blood grants resilience, tougher skin and elemental affinity.",
      "", "Dragon ancestor",
      "Black (acid)|Blue (lightning)|Brass (fire)|Bronze (lightning)|"
      "Copper (acid)|Gold (fire)|Green (poison)|Red (fire)|Silver (cold)|"
      "White (cold)" },
    { 9, BOOK_PHB, "Wild Magic",
      "Raw chaos: your magic sometimes surges beyond your control.",
      "", "", "" },

    /* --- warlock (29-31) --- */
    { 10, BOOK_PHB, "The Archfey",
      "A patron of the Feywild; charm, escape and beguilement.",
      "faerie fire, sleep|calm emotions, phantasmal force|"
      "blink, plant growth|dominate beast, greater invisibility|"
      "dominate person, seeming", "", "" },
    { 10, BOOK_PHB, "The Fiend",
      "A patron from the lower planes; fire, temptation and dark luck.",
      "burning hands, command|blindness/deafness, scorching ray|"
      "fireball, stinking cloud|fire shield, wall of fire|"
      "flame strike, hallow", "", "" },
    { 10, BOOK_PHB, "The Great Old One",
      "An alien intelligence; telepathy and psychic domination.",
      "dissonant whispers, Tasha's hideous laughter|"
      "detect thoughts, phantasmal force|clairvoyance, sending|"
      "dominate beast, Evard's black tentacles|"
      "dominate person, telekinesis", "", "" },

    /* --- wizard (32-39) --- */
    { 11, BOOK_PHB, "School of Abjuration",
      "Protective magic; an arcane ward absorbs damage for you.", "", "", "" },
    { 11, BOOK_PHB, "School of Conjuration",
      "Summoning and teleportation; conjure objects and blink between spaces.",
      "", "", "" },
    { 11, BOOK_PHB, "School of Divination",
      "Glimpse the future; portent replaces rolls with foreseen numbers.",
      "", "", "" },
    { 11, BOOK_PHB, "School of Enchantment",
      "Charm and compulsion; bend minds to your will.", "", "", "" },
    { 11, BOOK_PHB, "School of Evocation",
      "Elemental destruction, sculpted so allies are spared.", "", "", "" },
    { 11, BOOK_PHB, "School of Illusion",
      "Deception made real; illusions you can reshape at will.", "", "", "" },
    { 11, BOOK_PHB, "School of Necromancy",
      "Life and death; harvest life force and command the undead.", "", "", "" },
    { 11, BOOK_PHB, "School of Transmutation",
      "Change matter and form; a transmuter's stone grants shifting benefits.",
      "", "", "" },

    /* --- artificer (Tasha's) --- */
    { 12, BOOK_TCE, "Alchemist",
      "Potions and elixirs: healing, restoration and experimental concoctions.",
      "healing word, ray of sickness|flaming sphere, melf's acid arrow|"
      "gaseous form, mass healing word|blight, death ward|"
      "cloudkill, raise dead", "", "" },
    { 12, BOOK_TCE, "Armorer",
      "You wear arcane armor you can reshape as a guardian or an infiltrator.",
      "magic missile, thunderwave|mirror image, shatter|"
      "hypnotic pattern, lightning bolt|fire shield, greater invisibility|"
      "passwall, wall of force", "", "" },
    { 12, BOOK_TCE, "Artillerist",
      "You conjure an eldritch cannon that scorches, freezes or shields.",
      "shield, thunderwave|scorching ray, shatter|fireball, wind wall|"
      "ice storm, wall of fire|cone of cold, wall of force", "", "" },
    { 12, BOOK_TCE, "Battle Smith",
      "A defender: you fight beside a steel defender you built yourself.",
      "heroism, shield|branding smite, warding bond|"
      "aura of vitality, conjure barrage|aura of purity, fire shield|"
      "banishing smite, mass cure wounds", "", "" },

    /* ===================== Xanathar's Guide to Everything ================= */

    /* --- barbarian --- */
    { 0, BOOK_XGE, "Path of the Ancestral Guardian",
      "Ancestral spirits shield your allies and hinder those you strike.",
      "", "", "" },
    { 0, BOOK_XGE, "Path of the Storm Herald",
      "Your rage summons a storm aura shaped by the land that forged you.",
      "", "Storm aura", "Desert|Sea|Tundra" },
    { 0, BOOK_XGE, "Path of the Zealot",
      "A god's fury burns in you; death itself struggles to hold you.",
      "", "", "" },

    /* --- bard --- */
    { 1, BOOK_XGE, "College of Glamour",
      "Fey charm: you enthral audiences and command with a word.",
      "", "", "" },
    { 1, BOOK_XGE, "College of Swords",
      "A blade dancer whose flourishes are both performance and attack.",
      "", "Fighting style", "Dueling|Two-Weapon Fighting" },
    { 1, BOOK_XGE, "College of Whispers",
      "You trade in secrets and terror, poisoning minds with words.",
      "", "", "" },

    /* --- cleric --- */
    { 2, BOOK_XGE, "Forge Domain",
      "The smith's fire: you bless armour and weapons, and shrug off blows.",
      "identify, searing smite|heat metal, magic weapon|"
      "elemental weapon, protection from energy|fabricate, wall of fire|"
      "animate objects, creation", "", "" },
    { 2, BOOK_XGE, "Grave Domain",
      "The border between life and death; you deny it to others and cross it "
      "for your allies.",
      "bane, false life|gentle repose, ray of enfeeblement|"
      "revivify, vampiric touch|blight, death ward|"
      "antilife shell, raise dead", "", "" },

    /* --- druid --- */
    { 3, BOOK_XGE, "Circle of Dreams",
      "The Summer Court's blessing: healing rest and moonlit escape.",
      "", "", "" },
    { 3, BOOK_XGE, "Circle of the Shepherd",
      "You call totem spirits and speak for the beasts of the world.",
      "", "", "" },

    /* --- fighter --- */
    { 4, BOOK_XGE, "Arcane Archer",
      "Elven magic on the arrow: each shot can banish, char or ensnare.",
      "", "", "" },
    { 4, BOOK_XGE, "Cavalier",
      "A guardian of the mounted charge who marks and punishes foes.",
      "", "", "" },
    { 4, BOOK_XGE, "Samurai",
      "Unbending resolve: fighting spirit carries you past exhaustion.",
      "", "", "" },

    /* --- monk --- */
    { 5, BOOK_XGE, "Way of the Drunken Master",
      "You reel and stagger, turning apparent clumsiness into evasion.",
      "", "", "" },
    { 5, BOOK_XGE, "Way of the Kensei",
      "A weapon is an extension of the body; your chosen arms become deadly.",
      "", "", "" },
    { 5, BOOK_XGE, "Way of the Sun Soul",
      "You hurl your own life energy as searing light.",
      "", "", "" },

    /* --- paladin --- */
    { 6, BOOK_XGE, "Oath of Conquest",
      "Rule through fear: you crush resistance and bind foes in dread.",
      "armor of agathys, command|hold person, spiritual weapon|"
      "bestow curse, fear|dominate beast, stoneskin|"
      "cloudkill, dominate person", "", "" },
    { 6, BOOK_XGE, "Oath of Redemption",
      "Violence is the last resort; you shield others and turn enemies aside.",
      "sanctuary, sleep|calm emotions, hold person|"
      "counterspell, hypnotic pattern|otiluke's resilient sphere, stoneskin|"
      "hold monster, wall of force", "", "" },

    /* --- ranger --- */
    { 7, BOOK_XGE, "Gloom Stalker",
      "A hunter of the lightless places, striking before foes can react.",
      "disguise self|rope trick|fear|greater invisibility|seeming", "", "" },
    { 7, BOOK_XGE, "Horizon Walker",
      "A watcher of planar portals who steps between worlds mid-fight.",
      "protection from evil and good|misty step|haste|banishment|"
      "teleportation circle", "", "" },
    { 7, BOOK_XGE, "Monster Slayer",
      "You study a quarry's weaknesses and turn its own magic against it.",
      "protection from evil and good|zone of truth|magic circle|banishment|"
      "hold monster", "", "" },

    /* --- rogue --- */
    { 8, BOOK_XGE, "Inquisitive",
      "A detective's eye for tells, traps and the lie behind the answer.",
      "", "", "" },
    { 8, BOOK_XGE, "Mastermind",
      "You direct others, reading intentions and turning allies into weapons.",
      "", "", "" },
    { 8, BOOK_XGE, "Scout",
      "A skirmisher at home in the wild who slips away when pressed.",
      "", "", "" },
    { 8, BOOK_XGE, "Swashbuckler",
      "Duelling flair: you fight one on one and charm your way out of trouble.",
      "", "", "" },

    /* --- sorcerer --- */
    { 9, BOOK_XGE, "Divine Soul",
      "Celestial blood grants you the cleric's spell list alongside your own.",
      "", "Divine affinity", "Good|Evil|Law|Chaos|Neutrality" },
    { 9, BOOK_XGE, "Shadow Magic",
      "The Shadowfell answers you: hounds of ill omen and a body that endures.",
      "", "", "" },
    { 9, BOOK_XGE, "Storm Sorcery",
      "Wind and thunder attend your spells and carry you aloft.",
      "", "", "" },

    /* --- warlock --- */
    { 10, BOOK_XGE, "The Celestial",
      "An upper-planar patron: you channel healing light and radiant fire.",
      "cure wounds, guiding bolt|flaming sphere, lesser restoration|"
      "daylight, revivify|guardian of faith, wall of fire|"
      "flame strike, greater restoration", "", "" },
    { 10, BOOK_XGE, "The Hexblade",
      "A sentient weapon from the Shadowfell; your curse marks a foe for ruin.",
      "shield, wrathful smite|blur, branding smite|blink, elemental weapon|"
      "phantasmal killer, staggering smite|banishing smite, cone of cold",
      "", "" },

    /* --- wizard --- */
    { 11, BOOK_XGE, "War Magic",
      "A battle mage balancing arcane deflection against offensive power.",
      "", "", "" },

    /* ==================== Tasha's Cauldron of Everything ================== */

    /* --- barbarian --- */
    { 0, BOOK_TCE, "Path of the Beast",
      "A predator within: your rage grows claws, a bite or a lashing tail.",
      "", "Form of the Beast", "Bite|Claws|Tail" },
    { 0, BOOK_TCE, "Path of Wild Magic",
      "Raw magic leaks through your fury in unpredictable bursts.",
      "", "", "" },

    /* --- bard --- */
    { 1, BOOK_TCE, "College of Creation",
      "The Song of Creation: you sing objects and dancing items into being.",
      "", "", "" },
    { 1, BOOK_TCE, "College of Eloquence",
      "Flawless persuasion: your words never quite fail to land.",
      "", "", "" },

    /* --- cleric --- */
    { 2, BOOK_TCE, "Order Domain",
      "Law and obedience; you compel and command, and allies strike for you.",
      "command, heroism|hold person, zone of truth|mass healing word, slow|"
      "compulsion, locate creature|commune, dominate person", "", "" },
    { 2, BOOK_TCE, "Peace Domain",
      "Bonds of friendship that share harm and knit a party together.",
      "heroism, sanctuary|aid, warding bond|beacon of hope, sending|"
      "aura of purity, otiluke's resilient sphere|"
      "greater restoration, rary's telepathic bond", "", "" },
    { 2, BOOK_TCE, "Twilight Domain",
      "The comfort of night: darkvision, sanctuary from fear, and a twilight "
      "aura.",
      "faerie fire, sleep|moonbeam, see invisibility|"
      "aura of vitality, leomund's tiny hut|aura of life, greater invisibility|"
      "circle of power, mislead", "", "" },

    /* --- druid --- */
    { 3, BOOK_TCE, "Circle of Spores",
      "Decay as part of the cycle; necrotic spores and animated dead.",
      "chill touch|blindness/deafness, gentle repose|"
      "animate dead, gaseous form|blight, confusion|cloudkill, contagion",
      "", "" },
    { 3, BOOK_TCE, "Circle of Stars",
      "Starlight given form: you take a constellation's shape.",
      "", "Starry form", "Archer|Chalice|Dragon" },
    { 3, BOOK_TCE, "Circle of Wildfire",
      "Fire that clears the way for new growth, with a wildfire spirit bound "
      "to you.",
      "burning hands, cure wounds|flaming sphere, scorching ray|"
      "plant growth, revivify|aura of life, fire shield|"
      "flame strike, mass cure wounds", "", "" },

    /* --- fighter --- */
    { 4, BOOK_TCE, "Psi Warrior",
      "Psionic energy shields allies, shoves foes and sharpens your strikes.",
      "", "", "" },
    { 4, BOOK_TCE, "Rune Knight",
      "Giant runes carved into your gear, and the size to match.",
      "", "First rune", "Cloud|Fire|Frost|Stone" },

    /* --- monk --- */
    { 5, BOOK_TCE, "Way of Mercy",
      "A masked healer who can mend with one hand and harm with the other.",
      "", "", "" },
    { 5, BOOK_TCE, "Way of the Astral Self",
      "Spectral arms, a visage and a body of ki that fight alongside you.",
      "", "", "" },

    /* --- paladin --- */
    { 6, BOOK_TCE, "Oath of Glory",
      "The heroic ideal: you inspire feats of athletics and press the attack.",
      "guiding bolt, heroism|enhance ability, magic weapon|"
      "haste, protection from energy|compulsion, freedom of movement|"
      "commune, flame strike", "", "" },
    { 6, BOOK_TCE, "Oath of the Watchers",
      "A sentinel against extraplanar threats, alert and hard to surprise.",
      "alarm, detect magic|moonbeam, see invisibility|"
      "counterspell, nondetection|aura of purity, banishment|"
      "hold monster, scrying", "", "" },

    /* --- ranger --- */
    { 7, BOOK_TCE, "Fey Wanderer",
      "Fey mirth clings to you: your strikes carry dread and your words charm.",
      "charm person|misty step|dispel magic|dimension door|mislead", "", "" },
    { 7, BOOK_TCE, "Swarmkeeper",
      "A swarm of spirits accompanies you, biting, shifting and shielding.",
      "faerie fire, mage hand|web|gaseous form|arcane eye|insect plague",
      "", "" },

    /* --- rogue --- */
    { 8, BOOK_TCE, "Phantom",
      "Death clings to you; you harvest whispers from the slain.",
      "", "", "" },
    { 8, BOOK_TCE, "Soulknife",
      "Psionic blades and a mind that can speak across distance.",
      "", "", "" },

    /* --- sorcerer --- */
    { 9, BOOK_TCE, "Aberrant Mind",
      "An alien influence rewrote you; psionic spells come without components.",
      "arms of hadar, dissonant whispers, mind sliver|"
      "calm emotions, detect thoughts|hunger of hadar, sending|"
      "evard's black tentacles, summon aberration|"
      "rary's telepathic bond, telekinesis", "", "" },
    { 9, BOOK_TCE, "Clockwork Soul",
      "Order from Mechanus: you smooth out chance and restore balance.",
      "alarm, protection from evil and good|aid, lesser restoration|"
      "dispel magic, protection from energy|"
      "freedom of movement, summon construct|"
      "greater restoration, wall of force", "", "" },

    /* --- warlock --- */
    { 10, BOOK_TCE, "The Fathomless",
      "Something vast in the deep: a tentacle answers you and the water heals.",
      "create or destroy water, thunderwave|gust of wind, silence|"
      "lightning bolt, sleet storm|control water, summon elemental|"
      "bigby's hand, cone of cold", "", "" },
    { 10, BOOK_TCE, "The Genie",
      "A noble genie's pact, with a vessel to shelter in and elemental wrath.",
      "detect evil and good|phantasmal force|create food and water|"
      "phantasmal killer|creation",
      "Genie kind", "Dao (earth)|Djinni (air)|Efreeti (fire)|Marid (water)" },

    /* --- wizard --- */
    { 11, BOOK_TCE, "Bladesinging",
      "An elven tradition of sword and spell woven into one dance.",
      "", "", "" },
    { 11, BOOK_TCE, "Order of Scribes",
      "Your spellbook awakens; you rewrite spells and let the book take a hit.",
      "", "", "" },
};
const int SUBCLASS_COUNT = (int)(sizeof(SUBCLASSES) / sizeof(SUBCLASSES[0]));
