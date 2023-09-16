# Pokemon Gen3 to GenX
This is a multipurpose multibootable/cart-swappable homebrew designed for Pokémon Ruby/Sapphire/Emerald/Fire Red/Leaf Green.

It adds the ability to trade to the Generation 1 and 2 games, as well as other cartridges using this software.

It allows the user to run clock-based events in Pokémon Ruby/Sapphire/Emerald, even if the battery is dry.

It can also be used to fix issues present in the base games, like Espeon, Umbreon and the Pokérus being unobtainable in Fire Red/Leaf Green, or the Roamer IV glitch.

All communications need the DMG/GB/GBC Link Cable. The software is not compatible with the GBA Link Cable.

## Usage
### Starting - Loading a Cartridge
Start up the homebrew either through a flashcart, or multiboot, then select "Load Cartridge", and insert your Pokémon game.

If you multiboot with an already inserted cartridge, it's going to automatically read that.

At any moment, you may select "Swap Cartridge" in the Main Menu to load another Pokémon game up.

You can send the homebrew to other GBA consoles using the "Send Multiboot" option and a DMG/GB/GBC Link Cable.

### Trading
By adjusting the settings available in the Main Menu, you can choose which generation to trade with.
- Gen 1 is for trading with Pokémon Red/Blue/Green/Yellow.
- Gen 2 is for trading with Pokémon Gold/Silver/Crystal.
- Gen 3 is for trading with other GBA consoles running this homebrew.

The "Act as" option allows one to choose whether the console will act as the main unit or not. Both options will work fine, but it's suggested not to change this unless you know what you're doing.

#### Trading to Gen 1 or Gen 2
- If you want to trade to the Japanese versions of Gen 1 or Gen 2 games, you must set the Region option in the Main Menu to Jpn.
- The Pokémon received will be totally legitimate, barring their level and movesets.
- You will be able to freely change the nature of the received Pokémon during the trade.
- Trading across Generations is a lossy operation. As such, some information (like ribbons) will be lost!!!
- You will not be able to trade Pokémon which did not exist in the target games.
- These include: Unown ? and !, all Shiny Unown but I and V, certain Female Shiny Pokémon, Pokémon holding Mail.

### Clock Settings
In the Settings inside of the Main Menu, the Clock Settings will be available if you're playing Pokémon Ruby/Sapphire/Emerald.

Within the Clock Settings, you will be able to change various aspects of the game's clock.
- The Time of Day option allows the user to change what Eevee will evolve into.
- The Tide option influences the tide inside of Shoal Cave, which can be useful to catch Snorunt.
- By increasing the number of days, the user can make it so certain time-based events are properly run (like berries growing or the lottery).
- By enabling the RTC Reset Menu, the user can use the in-game secret RTC Reset Menu, which can be used to reset the clock in the event that the battery was changed.

All times reported within this menu assume a Dry Battery. If your battery is not dry, you may experience weird graphical behaviour, but the menu will still work fine.

### Party Options
By selecting the option View Party Gen X, you can see what you'll be trading to the other games.
- As a special option, you will be able to evolve Eevee into either Espeon or Umbreon.
- As a special option, you will be able to fix the IVs of Roamer Pokémon caught in Pokémon Ruby/Sapphire/Fire Red/Leaf Green.
- As a special option, if the "Tradeless Evo." Cheat option is enabled, you will be able to evolve Pokémon which would need to be traded in order to evolve.

### Settings
Customize how the homebrew behaves.
- System Language handles certain ambiguous cases, as well as the displayed name for Pokémon.
- Gen 1/2 Settings can be used to personalize the experience when trading with Gen 1 and Gen 2 games.
- Clock Settings can be used to alter the time of the games.
- Game Loaded can be used to specify which game is inserted. This option will only be available if the homebrew failed to detect the precise version.
- Color Settings can be used to customize how the homebrew looks.
- Cheats are special options which are not enabled by default, but may be useful for certain users.

#### Cheats
Inside the Cheats menu, you will find Special options which can be useful for various purposes.
- "Cross-Gen Evo." can be used to allow evolution of Pokémon which would not evolve in the target generation (A Scyther holding Metal Coat from Gen 1 will evolve with this option enabled).
- "Tradeless Evo." can be used to execute trade evolutions without a link cable.
- "Undistr. Events" can be used to make it so Celebi and Mew from Gen 1 and Gen 2 games will not be Japanese (but they will be illegal).
- "Fast Hatch Eggs" when enabled reduces the steps incoming eggs will need in order to hatch.
- "Give Pokérus to Party", can be used to fix the fact that it's impossible to get Pokérus in Pokémon Fire Red/Leaf Green.

## Credits
The following projects were useful while making this homebrew:

- [PKHeX](https://github.com/kwsch/PKHeX) - Understanding legitimacy checks.
- [PokeFinder](https://github.com/Admiral-Fish/PokeFinder) - Understanding RNG and calculations.
- [pret](https://github.com/pret) - Understanding the innermost workings of the games.

## License
All Pokémon names, sprites, names of related resources and the Japanese font are © Nintendo/Creatures Inc./GAME FREAK Inc.

Everything else, and the programming code, is governed by the MIT license.

## Example
[This is a Video showing how to use the homebrew](https://youtu.be/3-EKe_lQREY).


## Building
The easiest is to build with docker, since you don't have to install a bunch of tools and libraries
```bash
docker build . -t pokemon_gen3_to_genx
docker run --rm -it -v .:/pokemon_gen3_to_genx pokemon_gen3_to_genx
```
