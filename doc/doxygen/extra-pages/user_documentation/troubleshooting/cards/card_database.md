@page troubleshooting_card_database Card Database

[TOC]

Let's start with some terminology:

- Oracle is the tool that Cockatrice uses to parse data about cards from a .json file from a compatible source (e.g.
  MTGJson) and convert it into the cards.xml used by the card database.
- The card database is the backbone of the Cockatrice application. It reads its data from the cards.xml, tokens.xml, as
  well as any custom set .xmls defined in COCKATRICE_ROOT/customsets. It does this at startup and you can force the
  application to read from these files again by **[reloading the card database](\ref how_to_reload_the_card_database)**.
  If you have not modified the cards.xml
  externally or ran oracle (which automatically reloads the card database on success), this is unlikely to do anything.

# Is your problem with the database?

Let's start by identifying if your problem is related to the card database by establishing what the card database is
responsible for:

- The card database provides Cockatrice with definitions and properties of cards, sets, and tokens. It is not
  responsible for loading or displaying pictures.

A good way to check if your problem is related to the card database is this:

- Does the card show up in the cards.xml? (See \ref how_to_update_the_card_database)
- Does the card show up in the card database in the application when you search for it? (See \ref how_to_enable_sets)
- Does the card display information when examined through the "Text" or "Both" option of the card info widget? (If you
  have gotten this far, you most likely have a genuine issue with your card database.)
- Do different entries show up in the printing selector for the different sets? (See [Troubleshooting - Card Images](\ref troubleshooting_card_images)
  and \ref editing_decks_printings)

If your problem is not related to any of these questions, your issue is most likely not with the card database.

# How to reload the Card Database {#how_to_reload_the_card_database}

You can reload the card database by clicking "Card Database → Reload card database" action in the menu bar at the
top of the application.

It is highly unlikely that you ever have to do this manually unless you have modified the cards.xml externally while
Cockatrice was running. You most likely mean to [update the card database](\ref how_to_update_the_card_database).

# How to update the Card Database {#how_to_update_the_card_database}

You can update the card database by clicking on the "Help → Check for Card Updates..." action in the menu bar at the
top of the application.

## Update frequency

MTGJson is updated daily. See the [meta.json](https://mtgjson.com/api/v5/Meta.json) for information on when it was last
updated. If your Cockatrice version is >2.10.2, you can configure the application to automatically update the card
database every X days in the settings.

## Troubleshooting: Can't connect to MTGJson

If you can't connect to MTGJson or your computer is incapable of parsing the data from MTGJson itself, there is an
alternative mirror available at [Github - ebbit1q/mtgxml](https://github.com/ebbit1q/mtgxml).

It is a ready-to-use cards.xml generated automatically from the MTGJson data. This skips connection to MTGJson and the
parsing step. See the link for usage instructions.

# How to enable Sets {#how_to_enable_sets}

If your card database is up to date and you are still missing specific (but not all) cards, it is likely that you have
accidentally disabled the set. After each card database update, when the database has finished reloading, you are
prompted about wether you would like to enable newly found sets or not. Closing this dialog will disable the sets.

To re-enable the sets, click "Card Database → Manage Sets → Enable all sets" or click on the checkbox of your desired
set.