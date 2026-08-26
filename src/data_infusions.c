/* data_infusions.c -- Artificer Infusions (Tasha's Cauldron of Everything).
 *
 * Names, prerequisites and the kind of object each infusion needs come from
 * the book; the summaries are brief mechanical notes written for this
 * program. Replicate Magic Item is the one infusion that may be learned more
 * than once, and the wizard asks which item each copy replicates.
 */
#include "data.h"

const InfusionData INFUSIONS[] = {
{ "Arcane Propulsion Armor", "14th-level artificer", 14,
  "A suit of armor (requires attunement)",
  "Walking speed +5 feet; the armor's gauntlets are magic thrown weapons "
  "dealing 1d8 force damage, it cannot be removed against your will, and it "
  "replaces missing limbs." },

{ "Armor of Magical Strength", "", 2,
  "A suit of armor (requires attunement)",
  "6 charges: spend one to add your Intelligence modifier to a Strength "
  "check or save, or to stand from prone without spending movement." },

{ "Boots of the Winding Path", "6th-level artificer", 6,
  "A pair of boots (requires attunement)",
  "Teleport up to 15 feet to an unoccupied space you can see, as part of "
  "your movement." },

{ "Enhanced Arcane Focus", "", 2,
  "A rod, staff, or wand (requires attunement)",
  "+1 to spell attack rolls and you ignore half cover; the bonus rises to +2 "
  "at 10th level." },

{ "Enhanced Defense", "", 2,
  "A suit of armor or a shield",
  "+1 to Armor Class while worn or wielded; the bonus rises to +2 at "
  "10th level." },

{ "Enhanced Weapon", "", 2,
  "A simple or martial weapon",
  "+1 to attack and damage rolls; the bonus rises to +2 at 10th level." },

{ "Helm of Awareness", "10th-level artificer", 10,
  "A helmet (requires attunement)",
  "Advantage on initiative rolls, and you cannot be surprised while "
  "conscious." },

{ "Homunculus Servant", "", 2,
  "A gem or crystal worth at least 100 gp (requires attunement)",
  "Create a Tiny construct that obeys you, shares your proficiency bonus and "
  "can deliver a force-damage attack or channel your touch spells." },

{ "Mind Sharpener", "", 2,
  "A suit of armor or robes",
  "4 charges: as a reaction, spend one to succeed on a concentration saving "
  "throw you have just failed." },

{ "Radiant Weapon", "6th-level artificer", 6,
  "A simple or martial weapon (requires attunement)",
  "+1 to attack and damage, sheds bright light, and 4 charges to blind an "
  "attacker as a reaction." },

{ "Repeating Shot", "", 2,
  "A simple or martial weapon with the ammunition property",
  "+1 to attack and damage, and it produces its own ammunition, so it needs "
  "no reloading and ignores the loading property." },

{ "Replicate Magic Item", "", 2,
  "Varies by the item replicated",
  "Reproduce one common magic item, or one from the Replicable Items tables "
  "for your level. This infusion alone may be learned more than once." },

{ "Repulsion Shield", "6th-level artificer", 6,
  "A shield (requires attunement)",
  "+1 to Armor Class, and 4 charges to push an attacker 15 feet away as a "
  "reaction." },

{ "Resistant Armor", "6th-level artificer", 6,
  "A suit of armor (requires attunement)",
  "Resistance to one damage type of your choice: acid, cold, fire, force, "
  "lightning, necrotic, poison, psychic, radiant or thunder." },

{ "Returning Weapon", "", 2,
  "A simple or martial weapon with the thrown property",
  "+1 to attack and damage, and it returns to your hand immediately after "
  "it is thrown." },

{ "Spell-Refueling Ring", "6th-level artificer", 6,
  "A ring (requires attunement)",
  "Once per day, as an action, recover one expended spell slot of 3rd level "
  "or lower." },
};
const int INFUSION_COUNT = (int)(sizeof(INFUSIONS) / sizeof(INFUSIONS[0]));
